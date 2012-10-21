#include "config.h"
#include "search.h"
#include "libutil/trace.h"
#include "libdb/query.h"
#include "libmediadb/schema.h"
#include <errno.h>
#include <string.h>

// #define BOOST_SPIRIT_DEBUG 1

#if HAVE_BOOST_SPIRIT_INCLUDE_CLASSIC_HPP
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#else
#include <boost/spirit.hpp>
#include <boost/spirit/core.hpp>
#include <boost/spirit/tree/ast.hpp>
#include <boost/spirit/tree/tree_to_xml.hpp>
#endif

namespace upnpd {

#if HAVE_BOOST_SPIRIT_INCLUDE_CLASSIC_HPP
using namespace boost::spirit::classic;
#else
using namespace boost::spirit;
#endif

struct SearchGrammar: public grammar<SearchGrammar>
{
    enum {
	LITERAL,
	IDENTIFIER,
	STATEMENT,
	SSTATEMENT,
	CONJUNCTION,
	DISJUNCTION
    };
    
    template <typename Scanner>
    struct definition
    {
	definition(const SearchGrammar&)
	    : literal( leaf_node_d[lexeme_d[
				       ( chlit<>('\"') >>
					 *( strlit<>("\\\"") | (anychar_p - chlit<>('\"')) ) >>
					 chlit<>('\"')
					   )
				       | str_p("true")
				       | str_p("false")
			   ]]
		),
	      identifier( leaf_node_d[lexeme_d[
					  ((alpha_p | '@' | ':')
					   >> *(alnum_p | '@' | ':'))] ] ),
	
	      simplestatement(identifier
			       >> root_node_d[str_p("contains")
					      | str_p("doesNotContain")
					      | str_p("derivedfrom")
					      | str_p("exists")
					      | str_p("!=")
					      | str_p("<=")
					      | str_p(">=")
					      | ch_p('<')
					      | ch_p('>')
					      | ch_p('=')
				   ]
			       >> literal)
	{
	    statement
		= simplestatement
		| inner_node_d[ch_p('(') >> disjunction >> ch_p(')')];
	    
	    conjunction
		= statement >> *( root_node_d[str_p("and")] >> statement );
	    
	    disjunction
		= conjunction >> *( root_node_d[str_p("or")] >> conjunction );

#if 0
	    BOOST_SPIRIT_DEBUG_RULE(literal);
	    BOOST_SPIRIT_DEBUG_RULE(identifier);
	    BOOST_SPIRIT_DEBUG_RULE(statement);
	    BOOST_SPIRIT_DEBUG_RULE(simplestatement);
	    BOOST_SPIRIT_DEBUG_RULE(conjunction);
	    BOOST_SPIRIT_DEBUG_RULE(disjunction);
#endif
	}

	rule <Scanner, parser_context<>, parser_tag<LITERAL> > literal;
	rule <Scanner, parser_context<>, parser_tag<IDENTIFIER> > identifier;
	rule <Scanner, parser_context<>, parser_tag<SSTATEMENT> > simplestatement;
	rule <Scanner, parser_context<>, parser_tag<STATEMENT> > statement;
	rule <Scanner, parser_context<>, parser_tag<CONJUNCTION> > conjunction;
	rule <Scanner, parser_context<>, parser_tag<DISJUNCTION> > disjunction;

	rule <Scanner, parser_context<>, parser_tag<DISJUNCTION> > const&
	start() const { return disjunction; }
    };
};

static std::string UnparseQuotes(const std::string& s)
{
    std::string result;
    result.reserve(s.length());

    const char *p = s.c_str();
    while (*p)
    {
	if (*p == '\\' && p[1] != '\0')
	{
	    result += p[1];
	    ++p;
	}
	else if (*p != '\"')
	    result += *p;
	++p;
    }
    return result;
}

typedef char const*         iterator_t;
typedef tree_match<iterator_t> parse_tree_match_t;
typedef parse_tree_match_t::tree_iterator iter_t;

static const db::Query::Subexpression Translate(db::Query *qp, 
						const iter_t& i,
						unsigned int *collate)
{
//    TRACE << "In Translate value=\"" <<
//        std::string(i->value.begin(), i->value.end()) <<
//	"\" id=" << i->value.id().to_long() << 
//       " children.size()=" << i->children.size() << "\n";

    switch (i->value.id().to_long())
    {
    case SearchGrammar::DISJUNCTION:
    {
	db::Query::Subexpression lhs = Translate(qp, i->children.begin(), 
						 collate);
	db::Query::Subexpression rhs = Translate(qp, i->children.begin()+1,
						 collate);
	if (!lhs.IsValid())
	    return rhs;
	if (!rhs.IsValid())
	    return lhs;
	return qp->Or(lhs, rhs);
    }
    case SearchGrammar::CONJUNCTION:
    {
	db::Query::Subexpression lhs = Translate(qp, i->children.begin(), 
						 collate);
	db::Query::Subexpression rhs = Translate(qp, i->children.begin()+1,
						 collate);
//	TRACE << "Anding, lhs=" << lhs << " rhs=" << rhs << "\n";
	if (!lhs.IsValid())
	    return rhs;
	if (!rhs.IsValid())
	    return lhs;
	return qp->And(lhs, rhs);
    }
    case SearchGrammar::STATEMENT:
    case SearchGrammar::SSTATEMENT:
    {
	std::string field(i->children.begin()->value.begin(),
			  i->children.begin()->value.end());

	/* This ws-stripping shouldn't be necessary, but space_p as a
	 * skip parser doesn't seem to work in boost 1.35.0
	 */
	field = std::string(field, field.find_first_not_of(" \t"));
	field = std::string(field, 0, field.find_last_not_of(" \t")+1);

	unsigned int which = mediadb::FIELD_COUNT;
	if (field == "upnp:class")
	    which = mediadb::TYPE;
	else if (field == "upnp:artist")
	    which = mediadb::ARTIST;
	else if (field == "upnp:album")
	    which = mediadb::ALBUM;
	else if (field == "@id")
	    which = mediadb::ID;
	else if (field == "@parentID")
	    which = mediadb::IDPARENT;
	else if (field == "dc:title")
	    which = mediadb::TITLE;
	else if (field == "upnp:genre")
	    which = mediadb::GENRE;
	else if (field == "dc:date")
	    which = mediadb::YEAR;
	else if (field == "upnp:author")
	    which = mediadb::COMPOSER;
	else if (field == "upnp:author@role")
	    which = mediadb::COMPOSER; // Placeholder for later
	else
	{
//	    TRACE << "Can't do search field '" << field << "'\n";
	    return db::Query::Subexpression();
	}

	std::string literal((i->children.begin()+1)->value.begin(),
			    (i->children.begin()+1)->value.end());

	/* This ws-stripping shouldn't be necessary, but space_p as a
	 * skip parser doesn't seem to work in boost 1.35.0
	 */
	literal = std::string(literal, literal.find_first_not_of(" \t"));
	literal = std::string(literal, 0, literal.find_last_not_of(" \t")+1);

	db::RestrictionType rt = db::EQ;
	std::string op(i->value.begin(), i->value.end());
	if (op == "=")
	    rt = db::EQ;
	else if (op == "derivedfrom")
	    rt = db::EQ;
	else if (op == "!=")
	    rt = db::NE;
	else if (op == "<")
	    rt = db::LT;
	else if (op == ">")
	    rt = db::GT;
	else if (op == "<=")
	    rt = db::LE;
	else if (op == ">=")
	    rt = db::GE;
	else if (op == "contains")
	{
	    rt = db::LIKE;
	    literal = ".*" + literal + ".*";
	}
	else if (op == "exists")
	{
	    if (literal == "false")
		op = db::EQ;
	    else
		op = db::NE;
	    literal = "\"\"";
	}
	else
	{
	    TRACE << "Can't do search operator '" << op << "'\n";
	    return db::Query::Subexpression();
	}

//	TRACE << "field=" << field << " op=" << op << " lit=" << literal << "\n";

	literal = UnparseQuotes(literal);

	if (field == "upnp:author@role")
	{
	    /** author@role works very oddly, you can't really collate by it
	     */
	    if (literal == "Composer" && rt == db::EQ)
		return qp->Restrict(mediadb::COMPOSER, db::NE, "");
	    
	    TRACE << "Can't do query '" << field << "' " << op << " '"
		  << literal << "'\n";
	    
	    return db::Query::Subexpression();
	}

	if (which == mediadb::TYPE)
	{
	    unsigned int type = mediadb::TUNE;
	    if (literal == "object.container.playlistContainer")
		type = mediadb::PLAYLIST;
	    else if (literal == "object.container.storageFolder")
		type = mediadb::DIR;
	    else if (literal == "object.item.audioItem.musicTrack")
		type = mediadb::TUNE;
	    else if (literal == "object.item.audioItem.audioBroadcast")
		type = mediadb::RADIO;
	    else if (literal == "object.item.imageItem.photo")
		type = mediadb::IMAGE;
	    else if (literal == "object.container.person.musicArtist")
	    {
		qp->CollateBy(mediadb::ARTIST);
		*collate = mediadb::ARTIST;
		return qp->Restrict(mediadb::TYPE, db::EQ, mediadb::TUNE);
	    }
	    else if (literal == "object.container.genre.musicGenre")
	    {
		qp->CollateBy(mediadb::GENRE);
		*collate = mediadb::GENRE;
		return qp->Restrict(mediadb::TYPE, db::EQ, mediadb::TUNE);
	    }
	    else if (literal == "object.container.album.musicAlbum")
	    {
		qp->CollateBy(mediadb::ALBUM);
		*collate = mediadb::ALBUM;
		return qp->Restrict(mediadb::TYPE, db::EQ, mediadb::TUNE);
	    }

	    return qp->Restrict(which, rt, type);
	}

	return qp->Restrict(which, rt, literal);
    }
    default:
	break;
    }

    TRACE << "Don't like parse node\n";

    return db::Query::Subexpression();
}

unsigned int ApplySearchCriteria(db::Query *qp, const std::string& s,
				 unsigned int *collate)
{
    upnpd::SearchGrammar sg;

    tree_parse_info<> info = ast_parse(s.c_str(), sg >> end_p, space_p);

    if (!info.full)
    {
	TRACE << "Can't parse query\n";
	return EINVAL;
    }

    db::Query::Subexpression rep = Translate(qp, info.trees.begin(), collate);
    if (rep.IsValid())
    {
	unsigned int rc = qp->Where(rep);
	if (rc)
	{
	    TRACE << "DB refuses query\n";
	    return rc;
	}
    }

//    TRACE << "Query is " << qp->ToString() << "\n";
 
    return 0;
}

} // namespace upnpd

#ifdef TEST

#if HAVE_BOOST_SPIRIT_INCLUDE_CLASSIC_HPP
using namespace boost::spirit::classic;
#else
using namespace boost::spirit;
#endif

# include "libdbsteam/db.h"
# include "libmediadb/xml.h"

static void Test(db::Database *db, const char *s)
{
    upnpd::SearchGrammar sg;

    tree_parse_info<> info = ast_parse(s, sg >> end_p, space_p);

    if (!info.full)
    {
	TRACE << "Parsing failed\n";
	assert(info.full);
	return;
    }

    std::map<parser_id, std::string> rule_names;
    rule_names[upnpd::SearchGrammar::LITERAL] = "literal";
    rule_names[upnpd::SearchGrammar::IDENTIFIER] = "identifier";
    rule_names[upnpd::SearchGrammar::STATEMENT] = "statement";
    rule_names[upnpd::SearchGrammar::CONJUNCTION] = "conjunction";
    rule_names[upnpd::SearchGrammar::DISJUNCTION] = "disjunction";
//    tree_to_xml(std::cout, info.trees, s, rule_names);

    db::QueryPtr qp = db->CreateQuery();
    unsigned int collatefield;
    unsigned int rc = upnpd::ApplySearchCriteria(qp.get(), s, &collatefield);
    assert(rc == 0);
}

int main()
{
    db::steam::Database sdb(mediadb::FIELD_COUNT);
    sdb.SetFieldInfo(mediadb::ID, 
		     db::steam::FIELD_INT|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::PATH,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::ARTIST,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::ALBUM,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::GENRE,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);
    sdb.SetFieldInfo(mediadb::TITLE,
		     db::steam::FIELD_STRING|db::steam::FIELD_INDEXED);

    mediadb::ReadXML(&sdb, SRCROOT "/libmediadb/example.xml");

    Test(&sdb, "upnp:class derivedfrom \"object.item\"");
    Test(&sdb, "upnp:artist=\"Sting\" and upnp:album=\"Gold\"");
    Test(&sdb, "upnp:artist=\"Sting\" or upnp:album=\"Gold\" and upnp:genre exists false");
    Test(&sdb, "(upnp:artist=\"Sting\" or upnp:album=\"Gold\") and upnp:genre exists false");
    Test(&sdb, "(dc:date <= \"1999-12-31\" and dc:date >= \"1990-01-01\")");

    return 0;
}

#endif

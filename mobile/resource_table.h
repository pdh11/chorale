#ifdef __OBJC__

#import <UIKit/UIKit.h>
#import <UIKit/UITable.h>
#include <string>
#include <vector>

struct Resource
{
    std::string m_name;
};

@interface ResourceTable : UITable
{
    UITableColumn *m_column;
    std::vector<Resource> m_resources;
}

- (id)initWithFrame: (CGRect)frame;
- (void) dealloc;
- (int) numberOfRowsInTable: (UITable*)table;
- (UITableCell*) table: (UITable*)table cellForRow: (int)row column: (UITableColumn*)col;

@end

#endif //__OBJC__

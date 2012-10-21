#ifndef ENCODING_TASK_FLAC_H
#define ENCODING_TASK_FLAC_H

#include "encoding_task.h"

namespace import {

class EncodingTaskFlac: public EncodingTask
{
    unsigned int Run();

    explicit EncodingTaskFlac(const std::string& output_filename)
	: EncodingTask(output_filename) {}

public:
    static EncodingTaskPtr Create(const std::string& output_filename);
};

} // namespace import

#endif

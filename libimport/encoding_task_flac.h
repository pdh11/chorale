#ifndef ENCODING_TASK_FLAC_H
#define ENCODING_TASK_FLAC_H

#include "encoding_task.h"

namespace import {

class EncodingTaskFlac: public EncodingTask
{
    void Run();

    explicit EncodingTaskFlac(const std::string& output_filename)
	: EncodingTask(output_filename) {}

public:
    static EncodingTaskPtr Create(const std::string& output_filename);
};

};

#endif

#ifndef ENCODING_TASK_MP3_H
#define ENCODING_TASK_MP3_H

#include "encoding_task.h"

namespace import {

class EncodingTaskMP3: public EncodingTask
{
    void Run();

    explicit EncodingTaskMP3(const std::string& output_filename)
	: EncodingTask(output_filename) {}

public:
    static EncodingTaskPtr Create(const std::string& output_filename);
};

} // namespace import

#endif

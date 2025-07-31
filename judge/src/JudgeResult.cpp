#include "JudgeResult.hpp"

namespace Judge
{
    std::string TaskResult::toString() const
    {
        std::string task_result{ "Index: " + std::to_string(this->index) + "\n"};
        task_result.append("Exit code: " + std::to_string(this->exit_code) + "\n");
        task_result.append("Exit signal: " + std::to_string(this->exit_signal) + "\n");
        task_result.append("Time: " + std::to_string(this->time) + "\n");
        task_result.append("Wall time: " + std::to_string(this->wall_time) + "\n");
        task_result.append("Memory: " + std::to_string(this->memory) + "\n");
        task_result.append("Stdin: " + this->stdin.substr(0, this->stdin.size() < 10 ? this->stdin.size() : 10) + (this->stdin.size() > 10 ? "...\n" : "\n"));
        task_result.append("Expected output: " + this->expected_output.substr(0, this->expected_output.size() < 10 ? this->expected_output.size() : 10) + (this->expected_output.size() > 10 ? "...\n" : "\n"));
        task_result.append("Stdout: " + this->stdout.substr(0, this->stdout.size() < 10 ? this->stdout.size() : 10) + (this->stdout.size() > 10 ? "...\n" : "\n"));
        task_result.append("Stderr: " + this->stderr.substr(0, this->stderr.size() < 10 ? this->stderr.size() : 10) + (this->stderr.size() > 10 ? "...\n" : "\n"));
        task_result.append("Compile output: " + this->compile_output.substr(0, this->compile_output.size() < 10 ? this->compile_output.size() : 10) + (this->compile_output.size() > 10 ? "...\n" : "\n"));
        task_result.append("Status: " + std::string(TaskStatusToString(static_cast<TaskStatus>(this->status))));
        return task_result;
    }

    SubmissionResult::SubmissionResult(std::uint64_t id, int count)
    : m_SubmissionId{ id }
    , m_TaskCount{ count }
    , m_FinishedCount{ 0 }
    , m_TaskResults{}
    , m_Status{ SubmissionStatus::PENDING }
    , m_CreateAt{}
    , m_FinishAt{}
    {
        this->m_CreateAt = std::chrono::system_clock::now();
        this->m_TaskResults.resize(static_cast<std::size_t>(this->m_TaskCount));
    }

    bool SubmissionResult::insertTaskResult(TaskResult &&task_result)
    {
        bool finished{ false };
        ++this->m_FinishedCount;
        if (this->m_TaskCount == this->m_FinishedCount) {
            finished = true;
            this->m_FinishAt = std::chrono::system_clock::now();
            this->setStatus();
        }
        this->m_TaskResults.at(static_cast<size_t>(task_result.index)) = std::move(task_result);
        return finished;
    }

    void SubmissionResult::setStatus()
    {
        std::int16_t status_map = std::ranges::fold_left(
            this->m_TaskResults, 0, [](std::int16_t result, const auto &task_result) -> std::int16_t {
            return result | static_cast<std::int16_t>(task_result.status);
        });

        if (status_map == static_cast<std::int16_t>(TaskStatus::ACCEPTED)) {
            this->m_Status = SubmissionStatus::AC;
        } else if (status_map == static_cast<std::int16_t>(TaskStatus::COMPILATION_ERROR)) {
            this->m_Status = SubmissionStatus::CE;
        } else if (status_map & static_cast<std::int16_t>(TaskStatus::WRONG_ANSWER)) {
            this->m_Status = SubmissionStatus::WA;
        } else if (status_map & static_cast<std::int16_t>(TaskStatus::TIME_LIMIT_EXCEEDED)) {
            this->m_Status = SubmissionStatus::TLE;
        }  else if (status_map & static_cast<std::int16_t>(TaskStatus::INTERNAL_ERROR)) {
            this->m_Status = SubmissionStatus::IE;
        } else {
            this->m_Status = SubmissionStatus::RE;
        }
    }
}
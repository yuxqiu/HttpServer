#include "http/task.h"

Task::Task(std::function<void(Context &, const Task &)> &&f)
    : Task(std::move(f), nullptr) {}

Task::Task(std::function<void(Context &, const Task &)> &&f,
           std::unique_ptr<Task> &&next_task)
    : f(std::move(f)), next_task(std::move(next_task)) {}

void Task::next(Context &ctx) const {
  if (!next_task) {
    return;
  }
  f(ctx, *next_task.get());
}

void Task::drop() const {}

TaskList::TaskList()
    : header(std::make_unique<Task>(nullptr)), footer(nullptr) {}

TaskList::~TaskList() {
  while (header) {
    header = std::move(header->next_task);
  }
}

void TaskList::use(std::function<void(Context &, const Task &)> &&f) {
  // (only element is nullptr)
  if (footer == nullptr) {
    header = std::make_unique<Task>(std::move(f), std::move(header));
    footer = header.get();
  } else {
    footer->next_task =
        std::make_unique<Task>(std::move(f), std::move(footer->next_task));
    footer = footer->next_task.get();
  }
}

Task *TaskList::head() const { return header.get(); }
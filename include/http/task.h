#pragma once

#include "common.h"
#include "http/context.h"
#include <functional>
#include <memory>
#include <vector>

class Task {
private:
  std::function<void(Context &, const Task &)> f;

  std::unique_ptr<Task> next_task;

public:
  Task(std::function<void(Context &, const Task &)> &&f);
  Task(std::function<void(Context &, const Task &)> &&f,
       std::unique_ptr<Task> &&next_task);

  NOT_COPYABLE(Task);
  NOT_MOVEABLE(Task);

  // call current f and give it next Task
  void next(Context &ctx) const;

  // circumvent compiler checks
  void drop() const;

  friend class TaskList;
};

class TaskList {
private:
  std::unique_ptr<Task> header;
  Task *footer;

public:
  TaskList();
  ~TaskList();

  NOT_COPYABLE(TaskList);
  NOT_MOVEABLE(TaskList);

  Task *head() const;
  void use(std::function<void(Context &, const Task &)> &&f);
};
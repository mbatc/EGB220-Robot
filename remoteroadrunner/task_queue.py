import asyncio
from threading import Lock

class TaskQueue:
  '''
  TaskQueue implements a basic threaded task queue.
  Tasks added to the queue are executed sequentially.
  '''
  def __init__(self):
    self.worker = asyncio.create_task(self.worker())
    self.queue  = asyncio.Queue()
    self.running = True
    self.lock = Lock()
    pass

  def __del__(self):
    self.stop()

  def stop(self):
    '''
    Stop the task queue worker thread.
    '''
    self.running = False

  def enqueue(self, func, args = []):
    '''
    Add a task to the queue. Tasks are executed asychronously.
    '''
    self.queue.put_nowait([func, args])

  async def worker(self):
    '''
    A basic worker task which takes enqueued tasks and executes them
    '''
    while (self.running):
      task = await self.queue.get()

      func = task[0]
      args = task[1]
      if asyncio.iscoroutine(func) or asyncio.isfuture(func):
        await func
      else:
        func(*args)

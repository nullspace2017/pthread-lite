#include "pthread-lite.h"
#include <vector>
#include <cstring>
#include <cstdlib>
#include <iostream>

/** Define a thread-item class to hold data, etc private
* to each thread. For instance, this can store output from a thread that
* can be dumped to a file after all processing are done. This is useful
* because writing to a file in a multi-threaded program requires a mutex lock,
* thus halting work on other threads.
*/
class MyThreadItem {
  
  public:
    MyThreadItem(size_t id) : m_id(id), m_hit_counts(0) {}
  
    // example accessor for storing results for this thread
    void AddHits(size_t new_hits) { m_hit_counts += new_hits; } 
  
  // get the thread id
  size_t ID() const { return m_id; }

  // get the results back
  size_t Results() const { return m_hit_counts; }

  private:
    size_t m_id; // id to identify thread	

    // include any number of thread-specific data below
    size_t m_hit_counts; // results from all jobs processed on this thread
};


/** Define a work-item class to hold data for specific task
 * (eg some operation on a set of sequences stored in char array(
 */
class MyWorkItem {

  public:
    MyWorkItem(char* data) : m_data(data) {}
    
    // define the actual work to be done
    bool runStringProcessing(MyThreadItem* thread_data) {
       // do something with the data ... 
      size_t results = 0;
      if (m_data)
	results = strlen(m_data);  // some results from this operation...
      thread_data->AddHits(results);     // store the results in the thread-level store
      if (m_data) free(m_data);         // done with this unit, so clear data
    }   

    // always include a run function that takes only
    // a thread-item and returns bool
    bool run(MyThreadItem* thread_data) {
      // do the actual work
      return runStringProcessing(thread_data);
    }      

  private:

    // some chunk of data to be processed as one unit on one thread
    char * m_data;

};

int main() {	

  // create the work item queue and consumer threads    	   	
  WorkQueue<MyWorkItem*>  queue; // queue of work items to be processed by threads
  std::vector<ConsumerThread<MyWorkItem, MyThreadItem>* > threadqueue;

  // establish and start the threads
  size_t num_cores = 8;
  for (int i = 0; i < num_cores; ++i) {
    MyThreadItem * tu  = new MyThreadItem(i);  // create the thread-specific data. Must be on heap to make useful after this loop
    ConsumerThread<MyWorkItem, MyThreadItem>* threadr = // establish new thread to draw from queue
      new ConsumerThread<MyWorkItem, MyThreadItem>(queue, tu); // always takes WorkQueue and some thread item
    threadr->start(); 
    threadqueue.push_back(threadr); // add thread to the threadqueue
  }

  // add 1000 work jobs to the WorkQueue
  for (int i = 0; i < 1000; ++i) {

    // establish some chunk of data...
    char * data = (char*)malloc(1000); 
    for (size_t j = 0; j < 999; ++j)
      data[j] = 'a';
    data[999] = '\0';

    // add to work item and then to queue for processing
    // must be on heap, since dealloc is done inside ConsumerThread
    MyWorkItem * wu = new MyWorkItem(data);
    queue.add(wu);    
  } 


  // wait for the threads to finish
  for (int i = 0; i < num_cores; ++i) 
    threadqueue[i]->join();

  for (int i = 0; i < num_cores; ++i) {
    const MyThreadItem * td = threadqueue[i]->GetThreadData();
    std::cerr << "thread " << td->ID() << " results " << td->Results() << std::endl;
  }
  return 0;
}
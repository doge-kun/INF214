/* Alang is a collection of tools (functions, types, macros) for the INF214 class. 
   
   Current version: 1.0
*/

#ifndef _ALANG_HPP
#define _ALANG_HPP

#include <iostream>     // std::cout
#include <algorithm>    // std::random_shuffle
#include <vector>       // std::vector
#include <numeric>      // std::iota
#include <random>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <atomic>
#include <memory>

#include <type_traits>
#include <functional>
#include <mutex>
#include <condition_variable>

#include <chrono>
#include <ctime>

#include <cassert>

namespace alang {

  /* Macros for generating fresh variable names (can only be used once
     per line) */
  
#define COMBINE1(X,Y) X##Y  
#define COMBINE(X,Y) COMBINE1(X,Y)
#define VARNAME(X) COMBINE(X,__LINE__)

  /* Mainly for logging and printing, we define a global mutex and a
     few macros that signify a critical section with the global
     mutex. */

  std::mutex global_mutex;
  
#define enter_critical alang::global_mutex.lock();
#define exit_critical alang::global_mutex.unlock();

  // The following macro can be used in a block to make statements from the macro
  // to the end of the block crirical
#define CRITICAL std::lock_guard<std::mutex> sc_(global_mutex); 


  /* Helpers for printing to cout.  The log function takes any number
     of streamable arguments and prints them to cout.  The logl
     function does the same, and adds a newline.  Both functions are
     protected by global_mutex. */
   
  void _log() {};
  template <typename Streamable, typename ...Streamables> 
  void _log(const Streamable& s, const Streamables&... ss) {
    std::cout << s; _log(ss...);
  }

  template <typename ...Streamables> 
  void logl(const Streamables&... ss) {
    CRITICAL; _log(ss...); std::cout << std::endl;
  }
  template <typename ...Streamables> 
  void log(const Streamables&... ss) {
    CRITICAL; _log(ss...); std::cout << std::flush;
  }

  /* A simple RAII class: The parameter f is a function ran by the
     destructor */
  
  template <typename F>
  class scope_guard {
    F f;
  public:
    scope_guard(F f) : f(std::move(f)) {}
    scope_guard(const scope_guard&) = delete;
    ~scope_guard() { this->f(); }
  };
  template <typename F>
  scope_guard<F> make_scope_guard(F f) { return f; }

  /* Two classes for defining an integer range [begin, end) Usage: for
     (int i : range(0, 10)) ... */
                             
  class range_index {
    int i;
  public:
    range_index(int i) : i(i) {}
    operator int() { return this->i; }
    range_index& operator++() { ++i; return *this; }
    int operator*() { return this->i; }      
  };

  class range {
    int b, e;
  public:
    range(int b, int e) : b(b), e(e) {}
    range_index begin() { return this->b; }
    range_index end() { return this->e; }    
  };

  // If one uses range to just construct a loop, and the index is not used,
  // many compilers give a warning. With ignore(x) we can supress a warning about unused x.
  template <typename T> void ignore(const T& t) {};


  /* Helpers for random numbers, for sleeping a given or a random
     time, and measuring execution time. */
  
  // A globally available random number generator
  std::random_device rdevice;
  std::default_random_engine rengine(rdevice());
  
  // random integer between [0, n]
  int prandom(int n) {
    std::uniform_int_distribution<> dist(0, n);
    return dist(alang::rengine);
  }
  // random integer between [n, m]
  int prandom(int n, int m) {
    std::uniform_int_distribution<> dist(n, m);
    return dist(alang::rengine);
  }

  // sleep for t 
  template <class T> void sleep(T t) {
    std::this_thread::sleep_for(t);
  }
  // sleep for t milliseconds
  template <class T> void sleep_ms(T t) {
    sleep(std::chrono::milliseconds(t));
  }

  // sleep a random number of milliseconds, up to given limit
  void sleep_random(int uptoms) {
    sleep(std::chrono::milliseconds(prandom(uptoms)));
  }

  // time_ms(func) returns the execution time of func() in
  // milliseconds
  template <class F>
  auto time_ms(F f) {
    using namespace std::chrono;
    auto start = steady_clock::now();
    f();
    auto end   = steady_clock::now();
    return duration<double, std::milli>(end-start).count();
  }
  
  ////////////////
  // Semaphores //
  ////////////////
  

  /* A simple semaphore implementation to match the one in the
     book. */
  
  class semaphore {
  private:
    std::mutex mutex;
    std::condition_variable condition;
    int count;
    
  public:
    semaphore(int n=0) : count(n) {}
    void V() {
      {
        std::unique_lock<std::mutex> lock(this->mutex);
        ++(this->count);
      }
      this->condition.notify_all();
    }
    
    void P() {
      std::unique_lock<std::mutex> lock(this->mutex);
      while (this->count == 0) this->condition.wait(lock);
      --(this->count);
    }
  };

  //////////////
  // Monitors //
  //////////////
  
  /* A monitor implementation to match the book's definition The first
     statement in each procedure body in the monitor must be the macro
     invoation SYNC; We do not use a recursive mutex; a SYNCed
     procedure cannot call another SYNCed procedure in the same
     monitor. */
  
#define SYNC std::lock_guard<std::mutex> sc_(this->mutex_);  
  
  // The book's type name for condition variables is cond
  typedef std::condition_variable cond;

  // All monitors must inherit from this class
  class monitor {
  protected:
    monitor(const monitor&) = delete; // not to be copied    
    std::mutex mutex_; // all procedures are synchronized by this mutex    

    monitor() {}

    // The functions wait, signal, signal_all do not change the
    // monitor's state.  They are placed in the monitor base class to
    // bring them in scope in all the monitor's methods (to match the
    // book's syntax).

    void signal(cond& cv) { cv.notify_one(); }
    void signal_all(cond& cv) { cv.notify_all(); }

    void wait(cond& cv) {
      // when wait is called, the thread must own the monitor's mutex
     std::unique_lock<std::mutex> lk(this->mutex_, std::adopt_lock);
     cv.wait(lk);
     lk.release(); // release, so that lk's destructor does not unlock
                   // the monitor's mutex at scope exit      
    };
  };


    
  ///////////////
  // Processes //
  ///////////////

  /* A collection for threads.  If tc is such a collection, tc += f
     starts running f on a new thread. The destructor joins all
     threads that have been added. */

  class processes {
    std::vector<std::thread> threads;

  public:
    ~processes() { for (std::thread& ts : this->threads) ts.join(); }

    template <class F>
    void operator+=(F f) { threads.emplace_back(f); }
  };


  ///////////////////////////////////
  // Software Transactional Memory //
  ///////////////////////////////////

  // Memory transaction version number shared by all threads
  std::atomic<int> global_version(0);

  int current_global_version() { return global_version; }  
  int next_global_version() { return ++global_version; }

  // ForwardIterator::value_type should be pair<T, U>;
  // where T has a member named lock
  template <typename ForwardIterator>
  void release_many(ForwardIterator begin, ForwardIterator end) {
    while (begin != end) (begin++)->first->lock.unlock();
  }

  // ForwardIterator::value_type should be pair<T, U>;
  // where T has a member named lock
  template <typename ForwardIterator>
  bool lock_many(ForwardIterator begin, ForwardIterator end) {
    auto current = begin;
    while (current != end) {
      if (current->first->lock.try_lock()) { ++current; }
      else {
        // if cannot acquire a lock, release everything that was locked
        // so far
        release_many(begin, current);
        return false;
      }
    }
    return true;
  }

  // A base class for an entry in the STM
  struct ABase {
    mutable std::mutex lock;
    int version = 0;
  };  

  template <class T> class A; // will inherit from ABase

  // A 'run-time-polymorphic' wrapper type that holds any type; these are how
  // values are stored in the write_log before they are written to the STM
  class storeable_t {
  public:
    template <typename T> storeable_t(T x)
      : self(std::make_shared<model<T> >(std::move(x))) {}
 
    // this function assumes that T maches the instance model<T>.
    // If the types do not match --> undefined behavior
    template <typename T>
    const T& read() const {
      return static_cast<const storeable_t::model<T>*>(self.get())->data;
    }
    // this needs to be called so that type parameters match in
    // model<T> and A<T> (ABase's derived class). Otherwise undefined behavior
    void write_to(ABase* a) const { self->write_to(a); }
 
    template <typename T> friend class A;
 
  private:
    struct concept_t {
      virtual ~concept_t() = default;
      virtual const concept_t& read() const = 0;
      // read returns concept_t, not void*, so that we can make return types
      // covariant and use overriding
      virtual void write_to(ABase* a) const = 0;
    };    
 
    template <typename T> class model : public concept_t {
      public:
        model(T x) : data(std::move(x)) {}
        const model<T>& read() const { return *this; }
        void write_to(ABase* a) const {
          static_cast<A<T>*>(a)->data = this->data;
        } 

        T data;
    };
 
    std::shared_ptr<const concept_t> self;
  };

  // the exception type to be thrown when a transaction is aborted
  struct roll_back {};

  struct transaction {
    std::unordered_set<ABase*> read_log;
    std::unordered_map<ABase*, storeable_t> write_log;
    int read_version;
    bool active;
 
    transaction() : active(false) {}
    transaction(const transaction&) = delete; // making sure transactions not copyable
 
    void run(std::function<void()> code) {
      this->active = true;
      while (true) {
        this->read_version = current_global_version();
        read_log.clear();
        write_log.clear();
     
        try {
          code();
          this->try_commit();
          break;
        }
        catch (roll_back) {
        }
      }
      this->active = false;    
    }
 
   void try_commit() {
     if (!lock_many(this->write_log.begin(), this->write_log.end())) {
        throw roll_back();
     }

      int write_version = next_global_version();

      if (write_version > this->read_version + 1) { // no need for read validation, because
                                                    // no-other transactions have written anything anywhere
        for (ABase* r : this->read_log) {
          if (this->write_log.count(r) == 1) { // tests if r in write log
            if (r->version <= this->read_version) { // no need to lock, because we already
              // hold the lock (bcs r in write log)
              continue;
            }
            // otherwise jump to release_many
          } else {
            if (r->lock.try_lock()) {
              int v = r->version;
              r->lock.unlock();        
              if (v <= this->read_version) { continue; }
            } else {
              // this means rollback
            }
          }
     
          // getting here means rollback
          release_many(this->write_log.begin(), this->write_log.end());
          throw roll_back();
        }
      }
      for (auto e : this->write_log) {
        e.second.write_to(e.first); // e.first is the memory location, e.second the value stored in the log
        e.first->version = write_version;
        e.first->lock.unlock();
      }    
    }
  };

  //
  // Current Transaction
  //
  thread_local transaction current_transaction;
  
  void ATOMIC(std::function<void()> code) {
    if (!current_transaction.active) {
      current_transaction.run(code);
    } else code(); // if nested ATOMIC, just stay in the same transaction
  }

#define ATO ATOMIC([&]{ 
#define MIC })

#define ATO ATOMIC([&]{ 
#define MIC })

#define AWAIT(test) while (!(test)) { \
  std::uniform_int_distribution<> dist(0, 9); \
  sleep(std::chrono::milliseconds(dist(alang::rengine))); \
};

#define PP(S) ATO AWAIT(S > 0); S = S - 1; MIC;
#define VV(S) ATO S = S + 1; MIC;

  typedef A<int> sem;
  
  // A<T> holds a T that is tracked by STM
  template <class T>
  class A : public ABase {
    T data;
    friend class storeable_t::model<T>;
  public:
    A() = default;

    // copying happens by converting A<T> to T, then constructing from T
    A(const A<T>& t) : A(this->operator T()) {}

    // assignment happens by converting A<T> to T, then assigning from T    
    void operator=(const A<T>& t) { *this = static_cast<T>(t); }
    
    A(T t) : data(t) {}

    T read() const { return *this; }
    
    operator T() const {
      if (!current_transaction.active) { return this->data; }
      if (current_transaction.write_log.count(const_cast<A<T>*>(this)) == 1) {
        return current_transaction.write_log.at(const_cast<A<T>*>(this)).template read<T>();
      }
      if (!(this->lock.try_lock())) { throw roll_back(); }
      
      // version check must be while owning lock because version/data are not read atomically
      if (this->version > current_transaction.read_version) {
        this->lock.unlock(); // 
        throw roll_back(); 
      } else {
        T t = this->data;
        this->lock.unlock();
        current_transaction.read_log.insert(const_cast<A<T>*>(this));
        return t;
      } 
    }                                                                                           
    void operator=(T u) {
      if (!current_transaction.active) {
        current_transaction.run([this,&u]{ (*this)=u; });
      } else {
        // In C++20 we could do the following. Currently commented out since not all compilers support it yet
        // current_transaction.write_log.insert_or_assign(const_cast<A<T>*>(this), u);

        // the workaraound in C++17 are the following two lines
        auto r = current_transaction.write_log.insert({const_cast<A<T>*>(this), u});
        if (!r.second) (*(r.first)).second = u;        
      }
    }
  };
  
  /////////////////////////////////
  // Functions for running tasks //
  /////////////////////////////////
  
  std::vector<int> iota_randomized(int n) {
    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rengine);
    return indices;
  }
  
  void execute_concurrently(const std::vector<std::function<void()>>& fs) {
    std::vector<std::thread> threads;
    for (int i : iota_randomized(fs.size())) threads.emplace_back(fs[i]);
    for (auto& t : threads) t.join();
  }
  
  void execute_one(const std::vector<std::function<void()>>& fs) {
    std::uniform_int_distribution<> dist(0, fs.size()-1);
    fs[dist(rengine)]();
  }
  
  bool execute_one_conditional_task(
         const std::vector<std::function<bool()>>& conditions,
         const std::vector<std::function<void()>>& tasks) {
    for (int i : iota_randomized(tasks.size())) {
      if (conditions[i]()) { tasks[i](); return true; }
    }
    return false;
  }
  
  void collect_tasks(std::vector<std::function<void()>>& tasks) {}
  
  template <typename F, typename ...Fs>
  void collect_tasks(std::vector<std::function<void()>>& tasks,
                     const F& f, const Fs&... fs) {
    tasks.push_back(f);
    collect_tasks(tasks, fs...);
  }
  
  void collect_conditional_tasks(std::vector<std::function<bool()>>& conditions,
                                 std::vector<std::function<void()>>& tasks) {}
  
  template <typename C, typename F, typename ...Fs>
  void collect_conditional_tasks(std::vector<std::function<bool()>>& conditions,
                                 std::vector<std::function<void()>>& tasks,
                                 const C& c, const F& f, const Fs&... fs) {
    conditions.push_back(c);
    tasks.push_back(f);
    collect_conditional_tasks(conditions, tasks, fs...);
  }


  template <typename ...Fs>
  void CO(Fs ...fs) {
    std::vector<std::function<void()>> tasks;
    collect_tasks(tasks, fs...);
    execute_concurrently(tasks);
  }

  template <typename F>
  void CO(range r, F f) {
    std::vector<std::thread> threads;
    for (int i : r) threads.emplace_back(f, i);
    for (auto& t : threads) t.join();
  }
  
  template <typename ...Fs>
  void IF(Fs ...fs) {
    std::vector<std::function<bool()>> conditions;
    std::vector<std::function<void()>> tasks;  
    collect_conditional_tasks(conditions, tasks, fs...);
    execute_one_conditional_task(conditions, tasks);  
  }
  
  template <typename ...Fs>
  void DO(Fs ...fs) {
    std::vector<std::function<bool()>> conditions;
    std::vector<std::function<void()>> tasks;
    collect_conditional_tasks(conditions, tasks, fs...);
    while (execute_one_conditional_task(conditions, tasks)); 
  }
}  

using namespace std::chrono_literals;

using alang::A;
using alang::CO;
using alang::ATOMIC;

using alang::processes;
using alang::sleep;
using alang::sleep_random;
using alang::time_ms;
using alang::range;
using alang::semaphore;
using alang::sem;

using alang::monitor;
using alang::cond;

using alang::processes;

using alang::log;
using alang::logl;

#endif /* _ALANG_HPP */

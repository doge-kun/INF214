#ifndef _ALANG_HPP
#define _ALANG_HPP

/* This is a collection of tools (functions, types, macros) for the INF214 class. */

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


namespace alang {

#include <mutex>
#include <condition_variable>

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
  
// Global lock + critical section macros
  std::mutex global_mutex;
  
#define enter_critical alang::global_mutex.lock();
#define exit_critical alang::global_mutex.unlock();
  
  // Helpers for logging, delays etc.

  // A globally available random number generator
  std::random_device rdevice;
  std::default_random_engine rengine(rdevice());
  
  void _log() {};
  template <typename Streamable, typename ...Streamables> 
  void _log(const Streamable& s, const Streamables&... ss) { std::cout << s; _log(ss...); }

  template <typename ...Streamables> 
  void logl(const Streamables&... ss) { enter_critical; _log(ss...); std::cout << std::endl; exit_critical; }
  template <typename ...Streamables> 
  void log(const Streamables&... ss) { enter_critical; _log(ss...); std::cout << std::flush; exit_critical; }
  
  template <class T> void sleep(T t) {
    std::this_thread::sleep_for(t);
  }
  void sleep_random(int uptoms) {
    std::uniform_int_distribution<> dist(0, uptoms);
    sleep(std::chrono::milliseconds(dist(alang::rengine)));
  }
  
  template <class F>
  auto time_ms(F f) {
    using namespace std::chrono;
    auto start = steady_clock::now();
    f();
    auto end   = steady_clock::now();
    return duration<double, std::milli>(end-start).count();
  }
    
  // Macros for generating fresh variable names (can only be used once per line)
#define COMBINE1(X,Y) X##Y  
#define COMBINE(X,Y) COMBINE1(X,Y)
#define VARNAME(X) COMBINE(X,__LINE__)
  
  // Two classes for defining an integer range [begin, end)
  // Usage: for (int i : range(0, 10)) ...
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


  // A collection for threads.
  // If tc is a thread_container, tc += f starts running f on a new thread
  // The destructor joins all threads that have been added

  class thread_container {
    std::vector<std::thread> threads;
  public:

    template <class F>
    void operator+=(F f) { threads.emplace_back(f); }

    ~thread_container() { this->join(); }

    void join() {
      for (std::thread& ts : this->threads) ts.join();
      this->threads.clear();
    }
  };

  // an easier name for thread_container
  typedef alang::thread_container processes;


  // STM --------------------------------------------------------------

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
 
    template <typename T> struct model : concept_t {
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
        catch (roll_back r) {
        }
      }
      this->active = false;    
    }
 
   void try_commit() {
     if (!lock_many(this->write_log.begin(), this->write_log.end())) {
        throw roll_back();
     }

      int write_version = next_global_version();
   
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
      
      // maybe not strictly necessary to lock for the version check here
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
        current_transaction.write_log.insert_or_assign(const_cast<A<T>*>(this), u);
      }
    }
  };
  
  // Functions for running tasks ------------------------
  
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

#endif /* _ALANG_HPP */

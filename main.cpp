#include <stdio.h>

#include <boost/thread.hpp>
#include <boost/date_time.hpp>
#include <chrono>

#include <atomic>
//#include <cstddef>


#include "timer.h"



template<typename Element, size_t Size>
class CircularFifo{
public:
	enum{Capacity = Size + 1};
	CircularFifo():m_tail(0),m_head(0){}
	virtual ~CircularFifo(){}
	bool push(const Element& item);
	bool pop(Element& item);
	bool was_empty() const;
	bool was_full() const;
	bool is_lock_free() const;
private:
	size_t increment(size_t idx) const;
	std::atomic<size_t> m_tail;
	Element m_array[Capacity];
	std::atomic<size_t> m_head;
};

template<typename Element, size_t Size>
bool CircularFifo<Element, Size>::push(const Element& item){
	const auto current_tail = m_tail.load(std::memory_order_relaxed);
	const auto next_tail = increment(current_tail);
	if(next_tail != m_head.load(std::memory_order_acquire)){
		m_array[current_tail] = item;
		m_tail.store(next_tail, std::memory_order_release);
		return true;
	}else{
		// full queue
		return false;
	}
}

template<typename Element, size_t Size>
bool CircularFifo<Element,Size>::pop(Element& item){
	const auto current_head = m_head.load(std::memory_order_relaxed);
	if(current_head == m_tail.load(std::memory_order_acquire)){
	//if(current_head == m_tail.load()){
	//if(current_head == m_tail.load(std::memory_order_seq_cst)){
		// empty queue
		return false;
	}else{
		item = m_array[current_head];
		m_head.store(increment(current_head), std::memory_order_release);
		return true;
	}
}

template<typename Element, size_t Size>
bool CircularFifo<Element,Size>::was_empty() const{
	return (m_head.load() == m_tail.load());
}

template<typename Element, size_t Size>
bool CircularFifo<Element,Size>::was_full() const{
	const auto next_tail = increment(m_tail.load());
	return (next_tail == m_head.load());
}

template<typename Element, size_t Size>
bool CircularFifo<Element,Size>::is_lock_free() const{
	return (m_tail.is_lock_free() && m_head.is_lock_free());
}

template<typename Element, size_t Size>
size_t CircularFifo<Element,Size>::increment(size_t idx) const{
	return (idx+1)%Capacity;
}


//struct alignas(64) Message{
////struct alignas(16) Message{
	//uint64_t m_index;
	//uint64_t a1;
	//uint64_t a2;
	//uint64_t a3;
	//uint64_t a4;
	//uint64_t a5;
	//uint64_t a6;
	//uint64_t a7;
	//Message():m_index(0){
	//}
//};


// small queue message
struct alignas(64) Message{
//struct alignas(256) Message{
//struct Message{
	uint64_t m_index;
	//char padding64[3*8]; // 64
	//char padding128[64];
	//char padding256[128];

	//uint64_t a1;
	//uint64_t a2;
	//uint64_t a3;
	//uint64_t a4;
	//uint64_t a5;
	//uint64_t a6;
	//uint64_t a7;

	//uint64_t a8;
	//uint64_t a9;
	//uint64_t a10;
	//uint64_t a11;

	//uint64_t a12;
	//uint64_t a13;
	//uint64_t a14;
	//uint64_t a15; // why is this fast again??
	Message():m_index(0){
	}
};

// large queue message
//struct Message{
	//uint64_t m_index;
	//Message():m_index(0){
	//}
//};

// bigger message -> higher througputs
// smaller message -> lower latency
// 	but queues of sizes smaller than 40 can need cacheline aligned messages to maintain latency 
// 	queues of size 40 and bigger don't need cacheline aligned messages to be fast
//
// queue<UnalignedMessages,40>: 1 GiB -> 2.4 s
// queue<UnalignedMessages, 2>: 1 GiB -> 3.3 s
// queue<AlignedMessages,   2>: 1 GiB -> 2.4 s
//
// queues of size 1 are much slower in any case
//
// queue<AlignedMessages,   1>: 1 GiB -> 5.3 s 
// queue<UnalignedMessages, 1>: 1 GiB -> 5.0 s
//

CircularFifo<Message, 2> queue; // 
//CircularFifo<Message, 2048> queue; // 

alignas(64) uint64_t g_full_counter = 0;
alignas(64) uint64_t g_empty_counter = 0;
alignas(64) uint64_t g_push_counter = 0;
alignas(64) uint64_t g_pop_counter = 0;


//const size_t message_count = 1000000;

// 100 MiB
// 1024 byte = 1 KiB
// 1024^2 byte = 1 MiB
// 1024^3  byte = 1 GiB
//
// 1 message = 64 byte
// 

//const size_t message_count = 1024 * 1024 / 64; // 1 MiB -> 0.003 s
const size_t message_count = 1024 * 1024 * 1024 / 64; // 1 GiB -> 2.4 s
//const size_t message_count = 1024 * 1024 * 1024 / 64 * 10; // 10 GiB -> 24 s

// -> 500 MiB / s



void producer_thread_func(){
	Message m;
	for(uint64_t i = 0;g_push_counter < message_count; i++){
		m.m_index = i;

		if(false == queue.push(m) ){
			// queue was full
			//g_full_counter++;
			// random wait?
		}else{
			g_push_counter++;
			//m.m_index++;
		}
	}
}

void consumer_thread_func(){
	for(uint64_t i = 0;g_pop_counter < message_count; i++){
		Message m;
		if(false == queue.pop(m)){
			// queue was empty
			g_empty_counter++;
			// random wait?
		}else{
			g_pop_counter++;
			//printf("%lu ",m.m_index);
		}
	}
}



int main(int argc,char** argv){

	printf("sizeof(Message): %lu\n", sizeof(Message));

	// measure time
	Timer timer;
	timer.mark();

	boost::thread producer_thread(producer_thread_func);
	boost::thread consumer_thread(consumer_thread_func);
	producer_thread.join();
	consumer_thread.join();

	timer.mark();

	std::chrono::nanoseconds nanoseconds1 = timer.get_total_nanoseconds();
	uint64_t nanoseconds = nanoseconds1.count();

	printf("nanoseconds: %lu\n", nanoseconds);
	printf("microseconds: %lu\n", nanoseconds/1000);
	printf("milliseconds: %lu\n", nanoseconds/1000/1000);

	printf("full: %lu\n",g_full_counter);
	printf("empty: %lu\n",g_empty_counter);
	printf("push: %lu\n",g_push_counter);
	printf("pop: %lu\n",g_pop_counter);

	printf("\n");
	printf("push/full: %f\n", static_cast<double>(g_push_counter) / static_cast<double>(g_full_counter) );
	printf("pop/empty: %f\n", static_cast<double>(g_pop_counter) / static_cast<double>(g_empty_counter) );


	//printf("main: shutdown\n");
	printf("main: done\n");
	return 0;
}

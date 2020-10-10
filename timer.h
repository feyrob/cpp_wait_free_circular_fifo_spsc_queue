
#include <chrono>

class Timer{
public:
	std::vector< 
		std::chrono::time_point<
			std::chrono::high_resolution_clock
		> 
	> m_time_points;
	
	Timer(){
		m_time_points.reserve(11);
	}
	~Timer(){
	}

	std::chrono::time_point<
		std::chrono::high_resolution_clock
	>
	mark(){
		std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
		m_time_points.push_back(now);
		//if(m_time_points.size() +1 >= m_time_points.capacity()  ){
			//m_time_points.reserve( m_time_points.capacity() * 2 );
		//}

		return now;
	}

	std::chrono::nanoseconds
	//void
	get_total_nanoseconds(){
		//auto first_mark = m_time_points[0];
		auto first_mark = *m_time_points.begin();
		auto last_mark = m_time_points.back();

		auto delta = last_mark - first_mark;
		return delta;
	}
};

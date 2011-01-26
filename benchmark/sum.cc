#include "bitvector.h"
#include <vector>

#ifdef _WIN32
void gettimeofday(struct timeval* t, void* timezone)
{       struct _timeb timebuffer;
        _ftime(&timebuffer);
        t->tv_sec = timebuffer.time;
        t->tv_usec = 1000 * timebuffer.millitm;
}
#endif

#define printTimeDiff(str, a, b) \
    timeval a; \
    gettimeofday(&a, NULL); \
    std::cout << str << " ran " << a.tv_sec - b.tv_sec + (a.tv_usec - b.tv_usec) / 1000000.0 << " seconds." << std::endl; \
    gettimeofday(&a, NULL);

typedef uint8_t block_type;
typedef bitvector<block_type> bcv_t;
typedef block_type uncompressed_type;
const bcv_t::bit_count_t max_bit_width = sizeof(block_type)*8;
const bcv_t::size_t num_elements = 512ul * 1024 * 1024 / sizeof(block_type);

void insert_elementwise(bcv_t& v)
{
    for(bcv_t::size_t i = 0; i < num_elements; ++i) {
        v.push_back(i);
    }
};

bcv_t::size_t sum_loop(const bcv_t& v)
{
    bcv_t::size_t result = 0;
    for(bcv_t::size_t i = 0; i < num_elements; ++i) {
        result += v[i];
    }
    return result;
};

void fill_uncompressedVector(std::vector<uncompressed_type>& vec, bcv_t::bit_count_t bit_width)
{	
	bcv_t::block_t bitmask = ~(~0 << bit_width);
	vec.resize(num_elements);
    for(bcv_t::size_t i = 0; i < num_elements; ++i) {
        vec[i] = (i & bitmask);
    }
};

bcv_t::size_t sum_uncompressed(const std::vector<uncompressed_type>& vec)
{
	bcv_t::size_t result = 0;
    for(bcv_t::size_t i = 0; i < num_elements; ++i) {
        result += vec[i];
    }
    return result;
	//the following seems to be slower:
	//bcv_t::size_t result = 0;	
	//for(std::vector<uncompressed_type>::const_iterator it = vec.begin(); it != vec.end(); ++it)
	//{
	//	result += *it;
	//}
    //   return result;
};

bcv_t::size_t sum_iter(const bcv_t& v)
{
    bcv_t::size_t result = 0;
    const bcv_t::iterator end(v, v.size());
    for(bcv_t::iterator it = bcv_t::iterator(v); it != end; ++it) {
        result += *it;
    }
    return result;
};

bcv_t::size_t check_sum(bcv_t::bit_count_t bit_width)
{
    bcv_t::size_t result = 0;
    bcv_t::block_t bitmask = ~(~0 << bit_width);
    for(bcv_t::size_t i = 0; i < num_elements; ++i) {
        result += (i & bitmask);
    }
    return result;
};

int main(int, char**)
{
	#ifndef _WIN32
    if(!mlockall(MCL_CURRENT & MCL_FUTURE)) {
        std::cerr << "Can't memlock." << std::endl;
        exit(1);
    }
	#endif
    std::cout << "Maximum Memory usage: " << (num_elements * sizeof(block_type)) / (1024 * 1024) << "MB" << std::endl;
    
    std::cout << "sizeof(bitvector::size_t):  " << sizeof(bcv_t::size_t) << std::endl;
    std::cout << "sizeof(bitvector::block_t): " << sizeof(bcv_t::block_t) << std::endl;
    std::cout << "sizeof(bitvector::value_t): " << sizeof(bcv_t::value_t) << std::endl << std::endl;
    
    timeval t_start;
    gettimeofday(&t_start, NULL);
    for(bcv_t::bit_count_t bit_width = 1; bit_width <= max_bit_width; ++bit_width) {
        bcv_t v(bit_width, num_elements);
        printTimeDiff("init", t_init, t_start);
        
        insert_elementwise(v);
        printTimeDiff("set", t_set, t_init);
        
        v.inspect();

		std::vector<uncompressed_type> vec(num_elements);  
		fill_uncompressedVector(vec, bit_width);

        gettimeofday(&t_set, NULL);
        
        bcv_t::size_t result_loop = sum_loop(v);
        printTimeDiff("loop", t_read, t_set);
        
        bcv_t::size_t result_iter = sum_iter(v);
        printTimeDiff("iter", t_iter, t_read);

		bcv_t::size_t result_uncompressed = sum_uncompressed(vec);
        printTimeDiff("uncompressed", t_uncompressed, t_iter);
        
        std::cout << "sum of loop was:         " << result_loop << std::endl;
        std::cout << "sum of iter was:         " << result_iter << std::endl;
		std::cout << "sum of uncompressed was: " << result_uncompressed << std::endl;
        std::cout << "should be:               " << check_sum(bit_width) << std::endl << std::endl << std::endl;
        gettimeofday(&t_start, NULL);
		
    }
}

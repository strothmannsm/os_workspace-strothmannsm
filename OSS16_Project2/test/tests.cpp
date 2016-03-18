#include <stdio.h>
#include "gtest/gtest.h"
#include <pthread.h>
#include <cstring>

// Using a C library requires extern "C" to prevent function managling
extern "C" {
#include "../src/page_swap.c"
}

unsigned int score;

class GradeEnvironment : public testing::Environment {
	 public:
		// Override this to define how to set up the environment.
		virtual void SetUp() {
			srand(1337);
			score = 0;
		}
		// Override this to define how to tear down the environment.
		virtual void TearDown() {
			std::cout << "SCORE: " << score << std::endl;
		}
};

//page_request_result_t* approx_least_recently_used (const uint16_t page_number)
TEST (ALRU, BadInput) {
	initialize();

	page_request_result_t* par = approx_least_recently_used(2048,100);
	ASSERT_EQ(NULL,par);
	par = approx_least_recently_used(2049,100);
	ASSERT_EQ(NULL,par);

	destroy();
	score+=5;
}

TEST (ALRU, GoodInputNoPageFaults) {
	initialize();

	uint16_t page_number = 0;
	size_t clock_time = 0;
	size_t page_faults = 0;
	for (clock_time = 0; clock_time < 2048; ++clock_time) {
		
		page_number = rand() % 512;

		//std::cout << page_number << std::endl;

		page_request_result_t* prr = approx_least_recently_used(page_number,clock_time);
		if (prr != NULL){ 
			page_faults++;
			//std::cout << "RF = " << prr->frame_replaced <<  " RPage=" << prr->page_replaced<< " ReqPage=" << prr->page_requested << std::endl;
			free(prr);
		}
	}
	ASSERT_EQ(0,page_faults);
	
	destroy();
	score+=15;
}


//PageAlgorithmResults* least_frequently_used(const uint16_t page_number);
TEST (ALRU, GoodInputAllPageFaults) {
	initialize();

	uint16_t page_number = 0;
	size_t clock_time = 0;
	size_t page_faults = 0;
	for (clock_time = 0; clock_time < 101; ++clock_time) {
		
		page_number = rand() % 1024 + 512;

		//std::cout << page_number << std::endl;

		page_request_result_t* prr = approx_least_recently_used(page_number,clock_time);
		if (prr != NULL) { 
			page_faults++;
			//std::cout << "RF = " << prr->frame_replaced <<  " RPage=" << prr->page_replaced<< " ReqPage=" << prr->page_requested << std::endl;
			free(prr);
		}
	}
	ASSERT_EQ(101,page_faults);

	destroy();
	score+=25;
}

TEST (ALRU, GoodInputMixPageFaults) {
	initialize();

	size_t clock_time = 0;
	size_t page_faults = 0;
	for (clock_time = 0; clock_time < 2048; ++clock_time) {
		

		page_request_result_t* prr = approx_least_recently_used(clock_time,clock_time);
		if (prr != NULL) { 
			page_faults++;
			free(prr);
		}
	}
	ASSERT_EQ(1536,page_faults);

	destroy();
	score+=20;
}

//page_request_result_t* approx_least_recently_used (const uint16_t page_number)
TEST (LFU, BadInput) {
	initialize();

	page_request_result_t* par = least_frequently_used(2048,100);
	ASSERT_EQ(NULL,par);
	par = approx_least_recently_used(2049,100);
	ASSERT_EQ(NULL,par);

	destroy();
	score+=5;
}

TEST (LFU, GoodInputNoPageFaults) {
	initialize();

	uint16_t page_number = 0;
	size_t clock_time = 0;
	size_t page_faults = 0;
	for (clock_time = 0; clock_time < 2048; ++clock_time) {
		
		page_number = rand() % 512;

		//std::cout << page_number << std::endl;

		page_request_result_t* prr = least_frequently_used(page_number,clock_time);
		if (prr != NULL){ 
			page_faults++;
			//std::cout << "RF = " << prr->frame_replaced <<  " RPage=" << prr->page_replaced<< " ReqPage=" << prr->page_requested << std::endl;
			free(prr);
		}
	}
	ASSERT_EQ(0,page_faults);

	destroy();
	score+=10;
}


//PageAlgorithmResults* least_frequently_used(const uint16_t page_number);
TEST (LFU, GoodInputAllPageFaults) {
	initialize();

	uint16_t page_number = 0;
	size_t clock_time = 0;
	size_t page_faults = 0;
	for (clock_time = 0; clock_time < 101; ++clock_time) {
		
		page_number = rand() % 1024 + 512;

		//std::cout << page_number << std::endl;

		page_request_result_t* prr = least_frequently_used(page_number,clock_time);
		if (prr != NULL) { 
			page_faults++;
			//std::cout << "RF = " << prr->frame_replaced <<  " RPage=" << prr->page_replaced<< " ReqPage=" << prr->page_requested << std::endl;
			free(prr);
		}
	}
	ASSERT_EQ(101,page_faults);

	destroy();
	score+=30;
}

TEST (LFU, GoodInputMixPageFaults) {
	initialize();

	size_t clock_time = 0;
	size_t page_faults = 0;
	for (clock_time = 0; clock_time < 2048; ++clock_time) {
		

		page_request_result_t* prr = least_frequently_used(clock_time,clock_time);
		if (prr != NULL) { 
			page_faults++;
			free(prr);
		}
	}
	ASSERT_EQ(1536,page_faults);

	destroy();
	score+=20;
}

TEST (write_to_back_store, BadInputs) {
	initialize();
	char *baddata = NULL;
	char data[1024] = {0};
	
	ASSERT_EQ(false,write_to_back_store (data, 2048)); 	
	ASSERT_EQ(false,write_to_back_store (data, 2049)); 

	ASSERT_EQ(false,write_to_back_store (baddata, 10));

	destroy();
	score += 2;
}

TEST (write_to_back_store, GoodInput) {
	initialize();
	unsigned char data[1024];
	unsigned char read[1024];
	for (int i = 0; i < 2048; ++i) {
		for (int j = 0; j < 1024; ++j) {
			data[j] = rand() % 255;
		}
		bool res = write_to_back_store (data, i); 
		ASSERT_EQ(true,res);
		
		ASSERT_EQ(back_store_read(ps.bs,i+8,&read),true);
		ASSERT_EQ(0,memcmp(data,read,1024));
	}

	destroy();
	score += 8;
}

TEST (read_from_back_store, BadInputs) {
	initialize();
	char *baddata = NULL;
	char data[1024] = {0};

	ASSERT_EQ(false,read_from_back_store (data, 2048)); 	
	ASSERT_EQ(false,read_from_back_store (data, 2049)); 

	ASSERT_EQ(false,read_from_back_store (baddata, 10));

	destroy();
	score += 2;
}

TEST (read_from_back_store, GoodInput) {
	initialize();
	unsigned char data[1024];
	unsigned char read[1024];
	
	for (int i = 0; i < 2048; ++i) {
		for (int j = 0; j < 1024; ++j) {
			data[j] = rand() % 255;
		}

		ASSERT_EQ(back_store_write(ps.bs,i+8, &data),true);

		bool res = read_from_back_store (read, i); 
		ASSERT_EQ(true,res);
		ASSERT_EQ(0,memcmp(data,read,1024));

	}

	destroy();

	score += 8;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
		::testing::AddGlobalTestEnvironment(new GradeEnvironment);
		return RUN_ALL_TESTS();

}







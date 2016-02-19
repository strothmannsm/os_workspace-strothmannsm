#include <stdio.h>
#include "gtest/gtest.h"
#include <pthread.h>

// Using a C library requires extern "C" to prevent function managling
extern "C" {
	#include <dyn_array.h>
}
#include "../src/process_scheduling.c"

TEST (first_come_first_serve, nullInputProcessControlBlockDynArray) {
	ScheduleResult_t *sr = new ScheduleResult_t;
	dyn_array_t* pcbs = NULL;
	bool res = first_come_first_serve (pcbs,sr);
	EXPECT_EQ(false,res);
	delete sr;
}

TEST (first_come_first_serve, nullScheduleResult) {
	ScheduleResult_t *sr = NULL;
	dyn_array_t* pcbs = dyn_array_create(0,sizeof(ProcessControlBlock_t),NULL);
	bool res = first_come_first_serve (pcbs,sr);
	EXPECT_EQ(false,res);
	dyn_array_destroy(pcbs);
}

TEST (first_come_first_serve, goodInputA) {
	ScheduleResult_t *sr = new ScheduleResult_t;
	dyn_array_t* pcbs = dyn_array_create(0,sizeof(ProcessControlBlock_t),NULL);
	memset(sr,0,sizeof(ScheduleResult_t));
	// add PCBs now
	ProcessControlBlock_t data[3] = {
			[0] = {24,0},
			[1] = {3,0},
			[2] = {3,0}
	};
	// back loading dyn_array, pull from the back
	dyn_array_push_back(pcbs,&data[2]);
	dyn_array_push_back(pcbs,&data[1]);
	dyn_array_push_back(pcbs,&data[0]);	
	bool res = first_come_first_serve (pcbs,sr);	
	ASSERT_EQ(true,res);
	float answers[3] = {27,17,30};
	EXPECT_EQ(answers[0],sr->average_wall_clock_time);
	EXPECT_EQ(answers[1],sr->average_latency_time);
	EXPECT_EQ(answers[2],sr->total_run_time);
	dyn_array_destroy(pcbs);
	delete sr;
}

TEST (first_come_first_serve, goodInputB) {
	ScheduleResult_t *sr = new ScheduleResult_t;
	dyn_array_t* pcbs = dyn_array_create(0,sizeof(ProcessControlBlock_t),NULL);
	memset(sr,0,sizeof(ScheduleResult_t));
	// add PCBs now
	ProcessControlBlock_t data[4] = {
			[0] = {6,0},
			[1] = {8,0},
			[2] = {7,0},
			[3] = {3,0},
	};
	// back loading dyn_array, pull from the back
	dyn_array_push_back(pcbs,&data[3]);
	dyn_array_push_back(pcbs,&data[2]);
	dyn_array_push_back(pcbs,&data[1]);		
	dyn_array_push_back(pcbs,&data[0]);	
	bool res = first_come_first_serve (pcbs,sr);	
	ASSERT_EQ(true,res);
	float answers[3] = {16.25,10.25,24};
	EXPECT_EQ(answers[0],sr->average_wall_clock_time);
	EXPECT_EQ(answers[1],sr->average_latency_time);
	EXPECT_EQ(answers[2],sr->total_run_time);
	dyn_array_destroy(pcbs);
	delete sr;
}

/*
* ROUND ROBIN TEST CASES
*/
TEST (round_robin, nullInputProcessControlBlockDynArray) {
	ScheduleResult_t *sr = new ScheduleResult_t;
	dyn_array_t* pcbs = NULL;
	bool res = round_robin (pcbs,sr);
	EXPECT_EQ(false,res);
	delete sr;
}

TEST (round_robin, nullScheduleResult) {
	ScheduleResult_t *sr = NULL;
	dyn_array_t* pcbs = dyn_array_create(0,sizeof(ProcessControlBlock_t),NULL);
	bool res = round_robin (pcbs,sr);
	EXPECT_EQ(false,res);
	dyn_array_destroy(pcbs);
}

TEST (round_robin, goodInputA) {
	ScheduleResult_t *sr = new ScheduleResult_t;
	dyn_array_t* pcbs = dyn_array_create(0,sizeof(ProcessControlBlock_t),NULL);
	memset(sr,0,sizeof(ScheduleResult_t));
	// add PCBs now
	ProcessControlBlock_t data[3] = {
			[0] = {24,0},
			[1] = {3,0},
			[2] = {3,0}
	};
	// back loading dyn_array, pull from the back
	dyn_array_push_back(pcbs,&data[2]);
	dyn_array_push_back(pcbs,&data[1]);
	dyn_array_push_back(pcbs,&data[0]);	
	bool res = round_robin (pcbs,sr);	
	ASSERT_EQ(true,res);
	float answers[3] = {15.666667,3.666667,30};
	EXPECT_FLOAT_EQ(answers[0],sr->average_wall_clock_time);
	EXPECT_FLOAT_EQ(answers[1],sr->average_latency_time);
	EXPECT_EQ(answers[2],sr->total_run_time);
	dyn_array_destroy(pcbs);
	delete sr;
}

TEST (round_robin, goodInputB) {
	ScheduleResult_t *sr = new ScheduleResult_t;
	dyn_array_t* pcbs = dyn_array_create(0,sizeof(ProcessControlBlock_t),NULL);
	memset(sr,0,sizeof(ScheduleResult_t));
	// add PCBs now
	ProcessControlBlock_t data[4] = {
			[0] = {20,0},
			[1] = {5,0},
			[2] = {6,0}
	};
	// back loading dyn_array, pull from the back
	dyn_array_push_back(pcbs,&data[2]);
	dyn_array_push_back(pcbs,&data[1]);		
	dyn_array_push_back(pcbs,&data[0]);	
	bool res = round_robin (pcbs,sr);	
	ASSERT_EQ(true,res);
	float answers[3] = {22.333334,4,31};
	EXPECT_FLOAT_EQ(answers[0],sr->average_wall_clock_time);
	EXPECT_EQ(answers[1],sr->average_latency_time);
	EXPECT_EQ(answers[2],sr->total_run_time);
	dyn_array_destroy(pcbs);
	delete sr;
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}







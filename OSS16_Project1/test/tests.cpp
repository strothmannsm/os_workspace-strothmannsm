#include <stdio.h>
#include "gtest/gtest.h"
#include <pthread.h>

// Using a C library requires extern "C" to prevent function managling
extern "C" {
	#include <dyn_array.h>
}
#include "../src/process_scheduling.c"

#define NUM_PCB 30

unsigned int score;

class GradeEnvironment : public testing::Environment {
	 public:
		// Override this to define how to set up the environment.
		virtual void SetUp() {
			score = 0;
		}
		// Override this to define how to tear down the environment.
		virtual void TearDown() {
			std::cout << "SCORE: " << score << std::endl;
		}
};


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
	init_lock();
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

	score+=5;
}

TEST (first_come_first_serve, goodInputB) {
	init_lock();
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

	score+=5;
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
	init_lock();
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

	score+=5;
}

TEST (round_robin, goodInputB) {
	init_lock();
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

	score+=5;
}

/*
* LOAD PROCESS CONTROL BLOCKS TEST CASES
*/
TEST (load_process_control_blocks, nullFilePath) {
	dyn_array_t* da = load_process_control_blocks (NULL);
	ASSERT_EQ(da,(dyn_array_t*) NULL);

	score+=5;
}

TEST (load_process_control_blocks, notFoundFile) {

	dyn_array_t* da = load_process_control_blocks ("NotARealFile.Awesome");
	ASSERT_EQ(da,(dyn_array_t*)NULL);

	score+=5;
}

TEST (load_process_control_blocks, emptyFoundFile) {
	const char* fname = "EMPTYFILE.DARN";
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	int flags = O_CREAT | O_TRUNC | O_WRONLY;
	int fd = open(fname, flags, mode);
	close(fd);
	dyn_array_t* da = load_process_control_blocks (fname);
	ASSERT_EQ(da,(dyn_array_t*)NULL);

	score+=5;
}

TEST (load_process_control_blocks, incorrectPCBFoundFile) {
	const char* fname = "CANYOUHANDLETHE.TRUTH";
	uint32_t pcb_num = 10;
	uint32_t pcbs[5] = {1,2,3,4,5};
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	int flags = O_CREAT | O_TRUNC | O_WRONLY;
	int fd = open(fname, flags, mode);
	write(fd,&pcb_num,sizeof(uint32_t));
	write(fd,&pcbs,5 * sizeof(uint32_t));
	close(fd);
	dyn_array_t* da = load_process_control_blocks (fname);
	ASSERT_EQ(da,(dyn_array_t*)NULL);

	score+=5;
}

TEST (load_process_control_blocks, fullFoundFile) {
	const char* fname = "PCBs.bin";
	uint32_t pcb_num = NUM_PCB;
	uint32_t pcbs[NUM_PCB];
	for (uint32_t p = 0; p < pcb_num; ++p) {
		pcbs[p] = rand() % 15 + 1;
	}
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	int flags = O_CREAT | O_TRUNC | O_WRONLY;
	int fd = open(fname, flags, mode);
	write(fd,&pcb_num,sizeof(uint32_t));
	write(fd,&pcbs,pcb_num * sizeof(uint32_t));
	close(fd);
	dyn_array_t* da = load_process_control_blocks (fname);
	ASSERT_NE(da, (dyn_array_t*) NULL);
	for (size_t i = 0; i < dyn_array_size(da); ++i) {
		uint32_t* val = (uint32_t*) dyn_array_at(da,i);
		EXPECT_EQ(*val,pcbs[i]);
	}
	dyn_array_destroy(da);

	score+=10;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
		::testing::AddGlobalTestEnvironment(new GradeEnvironment);
		return RUN_ALL_TESTS();

}







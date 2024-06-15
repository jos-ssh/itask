#ifndef _INC_RANDOM_H_
#define _INC_RANDOM_H_

#define RAND_MAX 0x7FFFFFFF

extern int rand(void);
extern void srand(unsigned int seed);

#endif

#ifndef _dev_w3_org__2001_blindfold_iter
#define _dev_w3_org__2001_blindfold_iter

struct iter_s;

typedef void (iter_reset_function)(struct iter_s *iter);

struct iter_s {
    void *state;
    iter_reset_function *reset;
};

typedef struct iter_s iter;


/* void iter_reset(iter *i); */

/* 
   why do/while?  see Harbison/Steele p45 -- it works in more cases 
   but maybe we don't need it with this current if/then structure.
*/
#define iter_reset(i)               \
    if ((i)->state) {               \
    if ((i)->reset) (*((i)->reset))(i); \
    } else {                    \
    (i)->state = 0;             \
    }

#endif

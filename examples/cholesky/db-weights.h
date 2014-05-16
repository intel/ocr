#ifndef _DB_WEIGHTS_
#define _DB_WEIGHTS_

struct _dbWeightStruct gDbWeights[] = {

{"sequential_cholesky_task", {0, 0}, {100, 0}, {0, 0}, 100 },
{"mainEdt", {0, 0}, {100, 0}, {0, 0}, 100 },
{"trisolve_task", {0, 1}, {66, 34}, {1, 1}, 100 },
{"update_diagonal_task", {0, 1}, {66, 34}, {0, 0}, 100 },
{"update_nondiagonal_task", {0, 1}, {66, 33}, {0, 0}, 99 },

};

#endif

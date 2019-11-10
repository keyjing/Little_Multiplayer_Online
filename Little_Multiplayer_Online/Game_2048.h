#ifndef _Game_2048_h
#define _Game_2048_h

#define GAME_RES_SUCCESS	11			// 游戏状态
#define GAME_RES_CONTINUE	10
#define GAME_RES_FAILED		12

#define GAME_SIZE			4			// 行列大小
#define GAME_TARGET			2048		// 终点数字
#define GAME_SEED_CREATE	2			// 产生数字

const int GAME_ELEM_SIZE = GAME_SIZE * GAME_SIZE;

class Game_2048
{
public:
	Game_2048(int seed);
	~Game_2048() { }

	int get(int i, int j) const;

	int up(int seed) { col_decure(0, GAME_SIZE); return addSeedWithCheck(seed); }

	int down(int seed) { col_decure(GAME_SIZE - 1, -1); return addSeedWithCheck(seed); }

	int left(int seed) { row_decure(0, GAME_SIZE); return addSeedWithCheck(seed); }

	int right(int seed) { row_decure(GAME_SIZE - 1, -1); return addSeedWithCheck(seed); }

private:
	int addSeedWithCheck(int seed);			// 根据 seed 在矩阵空闲位置生成数字 并进行检查

	void row_decure(int start, int end);
	void col_decure(int start, int end);

	int matrix[GAME_SIZE][GAME_SIZE] = { {0} };
};


#endif

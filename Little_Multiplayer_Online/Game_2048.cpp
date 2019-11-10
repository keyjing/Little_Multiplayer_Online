#include "Game_2048.h"
#include<list>

using std::list;

Game_2048::Game_2048(int seed)
{
	int row1, col1, row2, col2;			// 将 seed 按位数分为两部分

	int bits = 0;
	int tmp = seed;
	while (tmp != 0) { ++bits; tmp = tmp >> 1; }

	tmp = 0;
	for (int i = 0; i < bits / 2; ++i)
		tmp = (tmp << 1) | 1;
	tmp = seed & tmp;
	row1 = (tmp / GAME_SIZE) % GAME_SIZE;
	col1 = tmp % GAME_SIZE;

	for (int i = bits / 2; i < bits; ++i)
		seed = seed >> 1;
	row2 = (seed / GAME_SIZE) % GAME_SIZE;
	col2 = seed % GAME_SIZE;

	if (row1 == row2 && col1 == col2)	// 重复检查
		col2 = (col2 + 1) % GAME_SIZE;

	matrix[row1][col1] = GAME_SEED_CREATE;
	matrix[row2][col2] = GAME_SEED_CREATE;
}

int Game_2048::get(int i, int j) const
{
	if (i < 0 || i >= GAME_SIZE || j < 0 || j >= GAME_SIZE)
		return 0;
	return matrix[i][j];
}

int Game_2048::addSeedWithCheck(int seed)
{
	int cnt = 0;
	list<int*> lstpos;
	for (int i = 0; i < GAME_SIZE; ++i)
	{
		for (int j = 0; j < GAME_SIZE; ++j)
		{
			if (matrix[i][j] == GAME_TARGET)
				return GAME_RES_SUCCESS;
			else if (matrix[i][j] == 0)
				lstpos.push_back(&matrix[i][j]);
		}
	}
	if (lstpos.empty())
		return GAME_RES_FAILED;
	seed = seed % lstpos.size();
	list<int*>::const_iterator it = lstpos.begin();
	while (seed-- > 0) ++it;
	**it = GAME_SEED_CREATE;
	return GAME_RES_CONTINUE;
}

void Game_2048::row_decure(int start, int end)
{
	for (int i = 0; i < GAME_SIZE; ++i)		// row
	{
		int j = start;
		int k = start;
		int ins = (start < end) ? 1 : -1;
		while (1)
		{
			while (j != end && matrix[i][j] == 0) j += ins;
			if (j == end) break;
			matrix[i][k] = matrix[i][j];
			if (k != j) matrix[i][j] = 0;
			j += ins;
			while (j != end && matrix[i][j] == 0) j += ins;
			if (j == end) break;
			if (matrix[i][j] == matrix[i][k])
			{
				matrix[i][k] *= 2;
				matrix[i][j] = 0;
			}
			k += ins;
		}
	}
}

void Game_2048::col_decure(int start, int end)
{
	for (int i = 0; i < GAME_SIZE; ++i)		// col
	{
		int j = start;
		int k = start;
		int ins = (start < end) ? 1 : -1;
		while (1)
		{
			while (j != end && matrix[j][i] == 0) j += ins;
			if (j == end) break;
			matrix[k][i] = matrix[j][i];
			if (k != j) matrix[j][i] = 0;
			j += ins;
			while (j != end && matrix[j][i] == 0) j += ins;
			if (j == end) break;
			if (matrix[j][i] == matrix[k][i])
			{
				matrix[k][i] *= 2;
				matrix[j][i] = 0;
			}
			k += ins;
		}
	}
}


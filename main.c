/*
MIT License
Copyright (c) 2020 someretical
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define ROWS 6
#define COLS 8
#define PATHS_PER_COL 1
#define MAX_PATH_LEN 4
#define PATH_DISTANCING 3

#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3
#define NO_DIR 4

typedef struct Tile
{
	int x;
	int y;

	int cave : 1;

	int pit : 1;
	int bat : 1;
	int wumpus : 1;

	int north : 1;
	int south : 1;
	int east : 1;
	int west : 1;
} Tile;

int getChar(Tile *t);
int getRandom(int lower, int upper);
int nx(int x);
int ny(int y);
void printMap(Tile map[COLS][ROWS]);
void initializeMap(Tile map[COLS][ROWS]);
void addPaths(Tile map[COLS][ROWS]);
void createPath(Tile map[COLS][ROWS], int *x, int *y, int *dir);
int getNullConnections(Tile *t, int dirs[4]);
int validateTile(Tile *t);
void fixPath(Tile map[COLS][ROWS], int x, int y, int dir);
void fixLoop(Tile map[COLS][ROWS], int x, int y);
void fixCave(Tile map[COLS][ROWS], int x, int y, int dir);
void fixMap(Tile map[COLS][ROWS]);
int populateMap(Tile map[COLS][ROWS]);
int generateMap(Tile map[COLS][ROWS]);

int getChar(Tile *t)
{
	if (t->cave)
	{
		if (!t->north)
			return 194; // ┬

		if (!t->south)
			return 193; // ┴

		if (!t->east)
			return 180; // ┤

		if (!t->west)
			return 195; // ├

		return 197; // ┼
	}

	if (!t->north)
	{
		if (!t->east)
			return 191; // ┐

		return 218; // ┌
	}

	if (!t->east)
		return 217; // ┘

	return 192; // └
}

int getRandom(int lower, int upper)
{
	// Don't do getRandom(0, 0);
	return (rand() % (upper - lower + 1)) + lower;
}

// Wrap around x values that are out of bounds
int nx(int x)
{
	return (x < 0) ? (x + COLS) : (x > COLS - 1) ? (x - COLS)
																							 : x;
}

// Wrap around y values that are out of bounds
int ny(int y)
{
	return (y < 0) ? (y + ROWS) : (y > ROWS - 1) ? (y - ROWS)
																							 : y;
}

void printMap(Tile map[COLS][ROWS])
{
	int x, y;

	printf("\n=== Map ===\n   ");

	for (x = 0; x < COLS; ++x)
		printf("%d", x);

	printf("\n\n");

	for (y = 0; y < ROWS; ++y)
	{
		printf("%d  ", y);

		for (x = 0; x < COLS; ++x)
			printf("%c", getChar(&map[x][y]));

		printf("\n");
	}

	printf("\n");
}

void initializeMap(Tile map[COLS][ROWS])
{
	int x, y;

	for (x = 0; x < COLS; ++x)
		for (y = 0; y < ROWS; ++y)
		{
			// Create new tile
			map[x][y].x = x;
			map[x][y].y = y;

			map[x][y].cave = 1;
			map[x][y].pit = 0;
			map[x][y].bat = 0;
			map[x][y].wumpus = 0;
		}

	for (x = 0; x < COLS; ++x)
		for (y = 0; y < ROWS; ++y)
		{
			// Link with tile east
			map[x][y].east = 1;
			map[nx(x + 1)][y].west = 1;

			// Link with tile south
			map[x][y].south = 1;
			map[x][ny(y + 1)].north = 1;
		}
}

void createPath(Tile map[COLS][ROWS], int *x, int *y, int *dir)
{
	Tile *path, *o = &map[*x][*y];
	int to;

	// Which side to extend from
	if (*dir == NO_DIR)
		*dir = getRandom(0, 3);

	// Paths can only go diagonally
	if (*dir == NORTH || *dir == SOUTH)
		to = getRandom(0, 1) ? EAST : WEST;
	else
		to = getRandom(0, 1) ? NORTH : SOUTH;

	if (*dir == NORTH)
	{
		path = &map[*x][ny(*y - 1)];
		path->cave = 0;

		// Create connection between origin tile and path tile north
		o->north = 1;
		path->south = 1;

		// Destroy connection between path and tile above path
		map[*x][ny(*y - 2)].south = 0;
		path->north = 0;

		// Destroy/add connection between path and tile east/west
		if (to == EAST)
		{
			map[nx(path->x - 1)][path->y].east = 0;
			path->west = 0;

			map[nx(path->x + 1)][path->y].west = 1;
			path->east = 1;
		}
		else
		{
			map[nx(path->x + 1)][path->y].west = 0;
			path->east = 0;

			map[nx(path->x - 1)][path->y].east = 1;
			path->west = 1;
		}
	}
	else if (*dir == EAST)
	{
		path = &map[nx(*x + 1)][*y];
		path->cave = 0;

		// Create connection between origin tile and path tile east
		o->east = 1;
		path->west = 1;

		// Destroy connection between path and tile east of path
		map[nx(*x + 2)][*y].west = 0;
		path->east = 0;

		// Destroy/add connection between path and tile north/south of path
		if (to == NORTH)
		{
			map[path->x][ny(path->y + 1)].north = 0;
			path->south = 0;

			map[path->x][ny(path->y - 1)].south = 1;
			path->north = 1;
		}
		else
		{
			map[path->x][ny(path->y - 1)].south = 0;
			path->north = 0;

			map[path->x][ny(path->y + 1)].north = 1;
			path->south = 1;
		}
	}
	else if (*dir == SOUTH)
	{
		path = &map[*x][ny(*y + 1)];
		path->cave = 0;

		// Create connection between origin tile and path tile south
		o->south = 1;
		path->north = 1;

		// Destroy connection between path and tile above path
		map[*x][ny(*y + 2)].north = 0;
		path->south = 0;

		// Destroy/add connection between path and tile east/west
		if (to == EAST)
		{
			map[nx(path->x - 1)][path->y].east = 0;
			path->west = 0;

			map[nx(path->x + 1)][path->y].west = 1;
			path->east = 1;
		}
		else
		{
			map[nx(path->x + 1)][path->y].west = 0;
			path->east = 0;

			map[nx(path->x - 1)][path->y].east = 1;
			path->west = 1;
		}
	}
	else if (*dir == WEST)
	{
		path = &map[nx(*x - 1)][*y];
		path->cave = 0;

		// Create connection between origin tile and path tile west
		o->west = 1;
		path->east = 1;

		// Destroy connection between path and tile west of path
		map[nx(*x - 2)][*y].east = 0;
		path->west = 0;

		// Destroy/add connection between path and tile north/south of path
		if (to == NORTH)
		{
			map[path->x][ny(path->y + 1)].north = 0;
			path->south = 0;

			map[path->x][ny(path->y - 1)].south = 1;
			path->north = 1;
		}
		else
		{
			map[path->x][ny(path->y - 1)].south = 0;
			path->north = 0;

			map[path->x][ny(path->y + 1)].north = 1;
			path->south = 1;
		}
	}

	*x = path->x;
	*y = path->y;
	*dir = to;
}

void addPaths(Tile map[COLS][ROWS])
{
	int x, y, i, r, rCounter, pointsCounter, dir, points[ROWS];

	// Add a new path(s) for each column
	for (x = 0; x < COLS; ++x)
		for (i = 0; i < PATHS_PER_COL; ++i)
		{
			dir = NO_DIR;

			// Path length can be 1,2,3
			r = getRandom(1, MAX_PATH_LEN);

			for (rCounter = 0; rCounter < r; ++rCounter)
			{
				// First iteration only
				if (rCounter == 0)
				{
					// Get a list of all non-path tiles in current column
					pointsCounter = 0;
					for (y = 0; y < ROWS; ++y)
						if (map[x][y].cave)
							points[pointsCounter++] = y;

					// Don't add a new path if there are no caves left
					if (pointsCounter == 0)
					{
						printf("No paths to create for column %d\n", x);
						rCounter = MAX_PATH_LEN;
						continue;
					}

					y = points[getRandom(0, pointsCounter - 1)];
				}

				createPath(map, &x, &y, &dir);
			}
		}

	// Don't want to generate too many long paths right next to each other
	if (r - PATH_DISTANCING > 0)
		x += r - PATH_DISTANCING;
}

int getNullConnections(Tile *t, int dirs[4])
{
	int c = 0;

	if (!t->north)
		dirs[c++] = NORTH;
	if (!t->east)
		dirs[c++] = EAST;
	if (!t->south)
		dirs[c++] = SOUTH;
	if (!t->west)
		dirs[c++] = WEST;

	return c;
}

int validateTile(Tile *t)
{
	if (t->cave)
		return (!t->north && t->east && t->south && t->west) ||
					 (t->north && !t->east && t->south && t->west) ||
					 (t->north && t->east && !t->south && t->west) ||
					 (t->north && t->east && t->south && !t->west) ||
					 (t->north && t->east && t->south && t->west);

	return (!t->north && !t->east && t->south && t->west) ||
				 (!t->north && t->east && t->south && !t->west) ||
				 (t->north && !t->east && !t->south && t->west) ||
				 (t->north && t->east && !t->south && !t->west);
}

void fixPath(Tile map[COLS][ROWS], int x, int y, int dir)
{
	map[x][y].cave = 1;

	if (dir == NORTH)
	{
		map[x][y].north = 1;
		map[x][ny(y - 1)].south = 1;
		map[x][ny(y - 1)].cave = 1;
	}
	else if (dir == EAST)
	{
		map[x][y].east = 1;
		map[nx(x + 1)][y].west = 1;
		map[nx(x + 1)][y].cave = 1;
	}
	else if (dir == SOUTH)
	{
		map[x][y].south = 1;
		map[x][ny(y + 1)].north = 1;
		map[x][ny(y + 1)].cave = 1;
	}
	else
	{
		map[x][y].west = 1;
		map[nx(x - 1)][y].east = 1;
		map[nx(x - 1)][y].cave = 1;
	}
}

void fixCave(Tile map[COLS][ROWS], int x, int y, int dir)
{
	if (dir == NORTH && map[x][ny(y - 1)].cave)
	{
		map[x][y].north = 1;
		map[x][ny(y - 1)].south = 1;
	}
	else if (dir == EAST && map[nx(x + 1)][y].cave)
	{
		map[x][y].east = 1;
		map[nx(x + 1)][y].west = 1;
	}
	else if (dir == SOUTH && map[x][ny(y + 1)].cave)
	{
		map[x][y].south = 1;
		map[x][ny(y + 1)].north = 1;
	}
	else if (dir == WEST && map[nx(x - 1)][y].cave)
	{
		map[x][y].west = 1;
		map[nx(x - 1)][y].east = 1;
	}
}

void fixLoop(Tile map[COLS][ROWS], int x, int y)
{
	int n, dir;

	n = getRandom(0, 3);

	if (n == 0)
		dir = getRandom(0, 1) ? NORTH : WEST;
	else if (n == 1)
		dir = getRandom(0, 1) ? NORTH : EAST;
	else if (n == 2)
		dir = getRandom(0, 1) ? EAST : SOUTH;
	else
		dir = getRandom(0, 1) ? SOUTH : WEST;

	if (n == 0)
		if (dir == NORTH)
		{
			map[x][y].north = 1;
			map[x][y].cave = 1;

			map[x][ny(y - 1)].south = 1;
			map[x][ny(y - 1)].cave = 1;
		}
		else
		{
			map[x][y].west = 1;
			map[x][y].cave = 1;

			map[nx(x - 1)][y].east = 1;
			map[nx(x - 1)][y].cave = 1;
		}
	else if (n == 1)
		if (dir == NORTH)
		{
			map[nx(x + 1)][y].north = 1;
			map[nx(x + 1)][y].cave = 1;

			map[nx(x + 1)][ny(y - 1)].south = 1;
			map[nx(x + 1)][ny(y - 1)].cave = 1;
		}
		else
		{
			map[nx(x + 1)][y].east = 1;
			map[nx(x + 1)][y].cave = 1;

			map[nx(x + 2)][y].west = 1;
			map[nx(x + 2)][y].cave = 1;
		}
	else if (n == 2)
		if (dir == SOUTH)
		{
			map[nx(x + 1)][ny(y + 1)].south = 1;
			map[nx(x + 1)][ny(y + 1)].cave = 1;

			map[nx(x + 1)][ny(y + 2)].north = 1;
			map[nx(x + 1)][ny(y + 2)].cave = 1;
		}
		else
		{
			map[nx(x + 1)][ny(y + 1)].east = 1;
			map[nx(x + 1)][ny(y + 1)].cave = 1;

			map[nx(x + 2)][ny(y + 1)].west = 1;
			map[nx(x + 2)][ny(y + 1)].cave = 1;
		}
	else if (dir == SOUTH)
	{
		map[x][ny(y + 1)].south = 1;
		map[x][ny(y + 1)].cave = 1;

		map[x][ny(y + 2)].north = 1;
		map[x][ny(y + 2)].cave = 1;
	}
	else
	{
		map[x][ny(y + 1)].west = 1;
		map[x][ny(y + 1)].cave = 1;

		map[nx(x - 1)][ny(y + 1)].east = 1;
		map[nx(x - 1)][ny(y + 1)].cave = 1;
	}
}

void fixMap(Tile map[COLS][ROWS])
{
	int x, y, c, n, dirs[4];

	// Fix caves and paths that are not valid
	for (x = 0; x < COLS; ++x)
		for (y = 0; y < ROWS; ++y)
		{
			while (!validateTile(&map[x][y]))
			{
				n = getRandom(0, getNullConnections(&map[x][y], dirs) - 1);

				// Link map[x][y] with another random tile adjacent to it that isn't linked with map[x][y] yet
				fixPath(map, x, y, dirs[n]);
			}
		}

	// Fix loops if MAX_PATH_LEN is greater than 3
	if (MAX_PATH_LEN > 3)
		for (x = 0; x < COLS; ++x)
			for (y = 0; y < ROWS; ++y)
				if (!map[x][y].cave && map[x][y].east && map[x][y].south &&																									// ┌
						!map[nx(x + 1)][y].cave && map[nx(x + 1)][y].west && map[nx(x + 1)][y].south &&													// ┐
						!map[nx(x + 1)][ny(y + 1)].cave && map[nx(x + 1)][ny(y + 1)].north && map[nx(x + 1)][ny(y + 1)].west && // ┘
						!map[x][ny(y + 1)].cave && map[x][ny(y + 1)].east && map[x][ny(y + 1)].north)														// └
					fixLoop(map, x, y);

	// Make sure all caves are connected to all adjacent caves
	for (x = 0; x < COLS; ++x)
		for (y = 0; y < ROWS; ++y)
			if (map[x][y].cave)
			{
				n = getNullConnections(&map[x][y], dirs);

				for (c = 0; c < n; ++c)
					fixCave(map, x, y, dirs[c]);
			}
}

int populateMap(Tile map[COLS][ROWS])
{
	int i, x, y, n, c, xs[COLS * ROWS], ys[COLS * ROWS];

	for (i = 0; i < 2; ++i)
	{
		c = 0;
		for (x = 0; x < COLS; ++x)
			for (y = 0; y < ROWS; ++y)
				if (map[x][y].cave && !map[x][y].pit)
				{
					xs[c] = x;
					ys[c++] = y;
				}

		n = getRandom(0, c - 1);
		map[xs[n]][ys[n]].pit = 1;

		printf("Placed pit at %d, %d\n", xs[n], ys[n]);
	}

	for (i = 0; i < 2; ++i)
	{
		c = 0;
		for (x = 0; x < COLS; ++x)
			for (y = 0; y < ROWS; ++y)
				if (!map[x][y].bat)
				{
					xs[c] = x;
					ys[c++] = y;
				}

		n = getRandom(0, c - 1);
		map[xs[n]][ys[n]].bat = 1;

		printf("Placed bat at %d, %d\n", xs[n], ys[n]);
	}

	c = 0;
	for (x = 0; x < COLS; ++x)
		for (y = 0; y < ROWS; ++y)
			if (map[x][y].cave)
			{
				xs[c] = x;
				ys[c++] = y;
			}

	n = getRandom(0, c - 1);
	map[xs[n]][ys[n]].wumpus = 1;

	printf("Placed wumpus at %d, %d\n", xs[n], ys[n]);

	c = 0;
	for (x = 0; x < COLS; ++x)
		for (y = 0; y < ROWS; ++y)
		{
			if (map[x][y].pit || map[x][y].wumpus || !map[x][y].cave ||
					(map[x][y].north && (map[x][ny(y - 1)].pit || map[x][ny(y - 1)].wumpus || map[x][ny(y - 1)].bat)) ||
					(map[x][y].east && (map[nx(x + 1)][y].pit || map[nx(x + 1)][y].wumpus || map[nx(x + 1)][y].bat)) ||
					(map[x][y].south && (map[x][ny(y + 1)].pit || map[x][ny(y + 1)].wumpus || map[x][ny(y + 1)].bat)) ||
					(map[x][y].west && (map[nx(x - 1)][y].pit || map[nx(x - 1)][y].wumpus || map[nx(x - 1)][y].bat)))
				continue;

			xs[c] = x;
			ys[c++] = y;
		}

	if (c == 0)
	{
		printf("LMAO can't generate player position RIP\n");
		return 1;
	}

	n = getRandom(0, c - 1);
	printf("Placed player at %d, %d\n", xs[n], ys[n]);

	return 0;
}

int generateMap(Tile map[COLS][ROWS])
{
	int r;

	initializeMap(map);
	printf("Initialized map\n");

	addPaths(map);
	printf("Added paths\n");

	fixMap(map);
	printf("Fixed map\n");

	r = populateMap(map);
	printf("Populated map\n");

	return r;
}

int main()
{
	Tile map[COLS][ROWS];

	// Seed the RNG
	srand(time(NULL));

	while (generateMap(map))
		;

	printMap(map);

	return 0;
}

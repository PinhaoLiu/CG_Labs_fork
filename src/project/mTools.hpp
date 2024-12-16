#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct Point
{
	glm::vec3 pos;
	float density;
	float pressure;
	glm::vec3 accel;
	glm::vec3 velocity;
	glm::vec3 velocity_eval;

	int next;
};

class PointBuffer
{
	using uint = unsigned int;
public:
	PointBuffer();
	void reset(uint capacity);
	uint size() const { return m_fluidCounts; }

	Point* get(uint index) { return m_FluidBuf + index; }
	const Point* get(uint index) const { return m_FluidBuf + index; }

	Point* addPointReuse();
	~PointBuffer();

private:
	Point* m_FluidBuf;
	uint m_fluidCounts;
	uint m_fluidBufCapacity;
};

class fBox3
{
public:
	fBox3() : min(glm::vec3(0.0f)), max(glm::vec3(0.0f)) {}
	fBox3(glm::vec3 aMin, glm::vec3 aMax) : min(aMin), max(aMax) {}
	glm::vec3 min, max;
};

class GridContainer
{
	using uint = unsigned int;
public:
	GridContainer() {}
	~GridContainer() {}

	void init(const fBox3& box, float sim_scale, float cell_size, float border, int* rex);
	void insertParticles(PointBuffer* pointBuffer);
	void findCells(const glm::vec3& p, float radius, int* gridCell);
	void findTwoCells(const glm::vec3& p, float radius, int* gridCell);
	int findCell(const glm::vec3& p);
	int getGridData(int gridIndex);

	const glm::ivec3* getGridRes() const { return &m_GridRes; }
	const glm::vec3* getGridMin(void) const { return &m_GridMin; }
	const glm::vec3* getGridMax(void) const { return &m_GridMax; }
	const glm::vec3* getGridSize(void) const { return &m_GridSize; }

	int getGridCellIndex(float px, float py, float pz);
	float getDelta() const { return m_GridDelta.x; }

private:
	std::vector<int> m_gridData;
	glm::vec3 m_GridMin;
	glm::vec3 m_GridMax;
	glm::ivec3 m_GridRes;
	glm::vec3 m_GridSize;
	glm::vec3 m_GridDelta;
	float m_GridCellSize;
};

#define MAX_NEIGHBOR_COUNTS 100
class NeighborTable
{
	using uint = unsigned int;
	using uchar = unsigned char;
	using ushort = unsigned short;
public:
	NeighborTable();
	void reset(ushort pointCounts);
	void point_prepare(ushort ptIndex);
	bool point_add_neighbor(ushort ptIndex, float distance);
	void point_commit();
	int getNeighborCounts(ushort ptIndex) { return m_pointExtraData[ptIndex].neighborCounts; }
	void getNeighborInfo(ushort ptIndex, int index, ushort& neighborIndex, float& neighborDistance);

	~NeighborTable();

private:
	union PointExtraData {
		struct {
			uint neighborDataOffset : 24;
			uint neighborCounts : 8;
		};
	};
	PointExtraData* m_pointExtraData;
	uint m_pointCounts;
	uint m_pointCapacity;

	uchar* m_neighborDataBuf;
	uint m_dataBufSize;
	uint m_dataBufOffset;

	ushort m_currPoint;
	int m_currNeighborCounts;
	ushort m_currNeighborIndex[MAX_NEIGHBOR_COUNTS];
	float m_currNeighborDistance[MAX_NEIGHBOR_COUNTS];

	void _growDataBuf(uint need_size);
};

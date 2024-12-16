#include "mTools.hpp"

#include <vector>

PointBuffer::PointBuffer() : m_FluidBuf(nullptr), m_fluidCounts(0), m_fluidBufCapacity(0)
{}

PointBuffer::~PointBuffer()
{
	delete[] m_FluidBuf;
	m_FluidBuf = nullptr;
}

void PointBuffer::reset(uint capacity)
{
	m_fluidBufCapacity = capacity;
	if (m_FluidBuf != nullptr) {
		delete[] m_FluidBuf;
		m_FluidBuf = nullptr;
	}
	if (m_fluidBufCapacity > 0) {
		m_FluidBuf = new Point[m_fluidBufCapacity]();
	}
	m_fluidCounts = 0;
}

Point* PointBuffer::addPointReuse()
{
	if (m_fluidCounts == m_fluidBufCapacity) {
		if (m_fluidBufCapacity * 2 > UINT16_MAX) {
			return m_FluidBuf + m_fluidCounts - 1;
		}
		m_fluidBufCapacity *= 2;
		Point* new_data = new Point[m_fluidBufCapacity]();
		memcpy(new_data, m_FluidBuf, m_fluidBufCapacity * sizeof(Point));
		delete[] m_FluidBuf;
		m_FluidBuf = new_data;
	}

	Point* point = m_FluidBuf + m_fluidCounts;
	m_fluidCounts++;

	point->pos = glm::vec3(0.0f);
	point->next = 0;
	point->velocity = glm::vec3(0.0f);
	point->velocity_eval = glm::vec3(0.0f);
	point->accel = glm::vec3(0.0f);
	point->density = 0.0f;
	point->pressure = 0.0f;

	return point;
}

void GridContainer::init(const fBox3& box, float sim_scale, float cell_size, float border, int* rex)
{
	float world_cellsize = cell_size / sim_scale;

	m_GridMin = box.min;
	m_GridMin -= border;
	m_GridMax = box.max;
	m_GridMax += border;
	m_GridSize = m_GridMax;
	m_GridSize -= m_GridMin;

	m_GridCellSize = world_cellsize;
	m_GridRes.x = (int)ceil(m_GridSize.x / world_cellsize);
	m_GridRes.y = (int)ceil(m_GridSize.y / world_cellsize);
	m_GridRes.z = (int)ceil(m_GridSize.z / world_cellsize);

	m_GridSize.x = m_GridRes.x * cell_size / sim_scale;
	m_GridSize.y = m_GridRes.y * cell_size / sim_scale;
	m_GridSize.z = m_GridRes.z * cell_size / sim_scale;

	m_GridDelta = m_GridRes;
	m_GridDelta /= m_GridSize;

	int gridTotal = (int)(m_GridRes.x * m_GridRes.y * m_GridRes.z);

	rex[0] = m_GridRes.x * 8;
	rex[1] = m_GridRes.y * 8;
	rex[2] = m_GridRes.z * 8;

	m_gridData.resize(gridTotal);
}

void GridContainer::insertParticles(PointBuffer* pointBuffer)
{
	std::fill(m_gridData.begin(), m_gridData.end(), -1);
	Point* p = pointBuffer->get(0);
	for (uint n = 0; n < pointBuffer->size(); n++, p++) {
		int gs = getGridCellIndex(p->pos.x, p->pos.y, p->pos.z);

		if (gs >= 0 && gs < m_gridData.size()) {
			p->next = m_gridData[gs];
			m_gridData[gs] = n;
		}
		else
			p->next = -1;
	}
}

void GridContainer::findCells(const glm::vec3& p, float radius, int* gridCell)
{
	for (int i = 0; i < 8; i++) gridCell[i] = -1;

	int sph_min_x = ((-radius + p.x - m_GridMin.x) * m_GridDelta.x);
	int sph_min_y = ((-radius + p.y - m_GridMin.y) * m_GridDelta.y);
	int sph_min_z = ((-radius + p.z - m_GridMin.z) * m_GridDelta.z);
	if (sph_min_x < 0) sph_min_x = 0;
	if (sph_min_y < 0) sph_min_y = 0;
	if (sph_min_z < 0) sph_min_z = 0;

	gridCell[0] = (sph_min_z * m_GridRes.y + sph_min_y) * m_GridRes.x + sph_min_x;
	gridCell[1] = gridCell[0] + 1;
	gridCell[2] = gridCell[0] + m_GridRes.x;
	gridCell[3] = gridCell[2] + 1;

	if (sph_min_z + 1 < m_GridRes.z) {
		gridCell[4] = gridCell[0] + m_GridRes.y * m_GridRes.x;
		gridCell[5] = gridCell[4] + 1;
		gridCell[6] = gridCell[4] + m_GridRes.x;
		gridCell[7] = gridCell[6] + 1;
	}
	if (sph_min_x + 1 >= m_GridRes.x) {
		gridCell[1] = -1;
		gridCell[3] = -1;
		gridCell[5] = -1;
		gridCell[7] = -1;
	}
	if (sph_min_y + 1 >= m_GridRes.y) {
		gridCell[2] = -1;
		gridCell[3] = -1;
		gridCell[6] = -1;
		gridCell[7] = -1;
	}
}

void GridContainer::findTwoCells(const glm::vec3& p, float radius, int* gridCell)
{
	for (int i = 0; i < 64; i++) gridCell[i] = -1;

	int sph_min_x = ((-radius + p.x - m_GridMin.x) * m_GridDelta.x);
	int sph_min_y = ((-radius + p.y - m_GridMin.y) * m_GridDelta.y);
	int sph_min_z = ((-radius + p.z - m_GridMin.z) * m_GridDelta.z);
	if (sph_min_x < 0) sph_min_x = 0;
	if (sph_min_y < 0) sph_min_y = 0;
	if (sph_min_z < 0) sph_min_z = 0;

	int base = (sph_min_z * m_GridRes.y + sph_min_y) * m_GridRes.x + sph_min_x;

	for (int z = 0; z < 4; z++) {
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				if ((sph_min_x + x >= m_GridRes.x) || (sph_min_y + y >= m_GridRes.y) || (sph_min_z + z >= m_GridRes.z))
					gridCell[16 * z + 4 * y + x] = -1;
				else
					gridCell[16 * z + 4 * y + x] = base + (z * m_GridRes.y + y) * m_GridRes.x + x;
			}
		}
	}
}

int GridContainer::findCell(const glm::vec3& p)
{
	int gc = getGridCellIndex(p.x, p.y, p.z);
	if (gc < 0 || gc > m_gridData.size()) return -1;
	return gc;
}

int GridContainer::getGridData(int gridIndex)
{
	if (gridIndex < 0 || gridIndex >= m_gridData.size())
		return -1;
	return m_gridData[gridIndex];
}

int GridContainer::getGridCellIndex(float px, float py, float pz)
{
	int gx = (int)((px - m_GridMin.x) * m_GridDelta.x);
	int gy = (int)((py - m_GridMin.y) * m_GridDelta.y);
	int gz = (int)((pz - m_GridMin.z) * m_GridDelta.z);
	return (gz * m_GridRes.y + gy) * m_GridRes.x + gx;
}

NeighborTable::NeighborTable() :
	m_pointExtraData(nullptr),
	m_pointCounts(0),
	m_pointCapacity(0),
	m_neighborDataBuf(nullptr),
	m_dataBufSize(0),
	m_dataBufOffset(0),
	m_currPoint(0),
	m_currNeighborCounts(0)
{
}

void NeighborTable::reset(ushort pointCounts)
{
	if (pointCounts > m_pointCapacity) {
		if (m_pointExtraData != nullptr) {
			delete[] m_pointExtraData;
			m_pointExtraData = nullptr;
		}
		m_pointExtraData = new PointExtraData[pointCounts]();
		m_pointCapacity = pointCounts;
	}
	m_pointCapacity = pointCounts;
	memset(m_pointExtraData, 0, m_pointCapacity * sizeof(PointExtraData));
	m_dataBufOffset = 0;
}

void NeighborTable::point_prepare(ushort ptIndex)
{
	m_currPoint = ptIndex;
	m_currNeighborCounts = 0;
}

bool NeighborTable::point_add_neighbor(ushort ptIndex, float distance)
{
	if (m_currNeighborCounts >= MAX_NEIGHBOR_COUNTS)
		return false;

	m_currNeighborIndex[m_currNeighborCounts] = ptIndex;
	m_currNeighborDistance[m_currNeighborCounts] = distance;
	m_currNeighborCounts++;
	return true;
}

void NeighborTable::point_commit()
{
	if (m_currNeighborCounts == 0) return;

	uint index_size = m_currNeighborCounts * sizeof(ushort);
	uint distance_size = m_currNeighborCounts * sizeof(float);

	if (m_dataBufOffset + index_size + distance_size > m_dataBufSize) {
		_growDataBuf(m_dataBufOffset + index_size + distance_size);
	}

	m_pointExtraData[m_currPoint].neighborCounts = m_currNeighborCounts;
	m_pointExtraData[m_currPoint].neighborDataOffset = m_dataBufOffset;

	memcpy(m_neighborDataBuf + m_dataBufOffset, m_currNeighborIndex, index_size);
	m_dataBufOffset += index_size;
	memcpy(m_neighborDataBuf + m_dataBufOffset, m_currNeighborDistance, distance_size);
	m_dataBufOffset += distance_size;
}

void NeighborTable::getNeighborInfo(ushort ptIndex, int index, ushort& neighborIndex, float& neighborDistance)
{
	PointExtraData neighborData = m_pointExtraData[ptIndex];
	ushort* indexBuf = (ushort*)(m_neighborDataBuf + neighborData.neighborDataOffset);
	float* distanceBuf = (float*)(m_neighborDataBuf + neighborData.neighborDataOffset + sizeof(ushort) * neighborData.neighborCounts);

	neighborIndex = indexBuf[index];
	neighborDistance = distanceBuf[index];
}

NeighborTable::~NeighborTable()
{
	if (m_pointExtraData != nullptr) {
		delete[] m_pointExtraData;
		m_pointExtraData = nullptr;
	}
	if (m_neighborDataBuf != nullptr) {
		delete[] m_neighborDataBuf;
		m_neighborDataBuf = nullptr;
	}
}

void NeighborTable::_growDataBuf(uint need_size)
{
	uint newSize = m_dataBufSize > 0 ? m_dataBufSize : 1;
	while (newSize < need_size) {
		newSize *= 2;
	}
	if (newSize < 1024) {
		newSize = 1024;
	}
	uchar* newBuf = new uchar[newSize]();
	if (m_neighborDataBuf != nullptr) {
		memcpy(newBuf, m_neighborDataBuf, m_dataBufSize);
		delete[] m_neighborDataBuf;
	}
	m_neighborDataBuf = newBuf;
	m_dataBufSize = newSize;
}

#pragma once

#include "mTools.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


class FluidSystem
{
	using uint = unsigned int;
	using ushort = unsigned short;
	using v3 = glm::vec3;
public:
	FluidSystem();
	void init(ushort maxPointCounts,
		const v3& wallBox_min, const v3& wallBox_max,
		const v3& initFluidBox_min, const v3& initFluidBox_max,
		const v3& gravity)
	{
		_init(maxPointCounts, fBox3(wallBox_min, wallBox_max),
			fBox3(initFluidBox_min, initFluidBox_max), gravity);
	}
	uint getPointStride() const { return sizeof(Point); }
	uint getPointCounts() const { return m_pointBuffer.size(); }
	const Point* getPointBuf() const { return m_pointBuffer.get(0); }
	void tick();

	~FluidSystem();

private:
	void _init(ushort maxPointCounts, const fBox3& wallBox,
		const fBox3& initFluidBox, const v3& gravity);
	void _computePressure();
	void _computeForce();
	void _advance();
	void _addFluidVolume(const fBox3& fluidBox, float spacing);

	PointBuffer m_pointBuffer;
	GridContainer m_gridContainer;
	NeighborTable m_neighborTable;

	std::vector<float> posData;

	float m_kernelPoly6;
	float m_kernelSpiky;
	float m_kernelViscosity;

	float m_pointDistance;
	float m_unitScale;
	float m_viscosity;
	float m_restDensity;
	float m_pointMass;
	float m_smoothRadius;
	float m_gasConstantK;
	float m_boundaryStiffness;
	float m_boundaryDampening;
	float m_speedLimiting;
	v3 m_gravityDir;

	int m_rexSize[3];

	fBox3 m_sphWallBox;
};

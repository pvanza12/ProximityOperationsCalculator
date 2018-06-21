#pragma once
#include "ObjectFiles\ObjectFiles\ObjectFiles.h"
#include "Eigen\Dense"
#ifdef PROXOPS_EXPORTS
#define PROXOPS_API __declspec(dllexport)
#else
#define PROXOPS_API __declspec(dllimport)
#endif

using namespace Eigen;

namespace ProxOps
{
	class Functions
	{
	public:
		static PROXOPS_API void PromityOperations(Objects::Asset assetObject, Objects::Target targetObject, double simTime_mjd, Objects::satCmdMessage* outMessages);

		static PROXOPS_API void ProxOpsMinimization(double* r_dep_Eci_km, double* v_dep_Eci_kms, double* r_ch_Eci_km, double* v_ch_Eci_kms, bool sunConstraint, double time_mjd,
			rpoType type, double* parameters, double* dv1, double* dv2, double& t_f, bool ipopt);

	};
}
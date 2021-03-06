// ProximityOperations.cpp : Defines the exported functions for the DLL application.
// Current 1 Feb 18

#include "stdafx.h"
#include "ProximityOperations.h"
#include "Propagator\Propagator\Propagator.h"
#include <iostream>
using namespace std;

const double pi = 3.14159265359;
const double mu = 3.986004418e5;


double BurnEstimator(double dryMass_kg, double fuel_kg, double Isp_s, double massRate_kgs, double deltaV_ms)
{
	double mf, m0;
	const double g0 = 9.80665;
	m0 = dryMass_kg + fuel_kg;
	mf = m0 / log(deltaV_ms / (g0*Isp_s));
	return (m0 - mf) / massRate_kgs;
}

double meanMotionCalc(Vector3d r, Vector3d v)
{
	double a = pow(2 / r.norm() - pow(v.norm(), 2) / mu, -1);
	return sqrt(mu / a / a / a);
}

void Hcw2InertialBurn(Vector3d rChEciKm, Vector3d vChEciKms, Vector3d, Vector3d rDepHcwKm, Vector3d vDepEciKms, Vector3d v1)
{
	Vector3d rDepEciKm, vEciKms; 
	// Convert everything back to inertial to calculate inertial delta-V requirements
	Ric2Eci(rChEciKm, vChEciKms, rDepEciKm, vEciKms, rDepHcwKm, v1);
	//cout << "vEciKms \n" << vEciKms << endl;
	v1 = vEciKms - vDepEciKms;

	// Propagate chief to final burn and calculate second dV
	Propagator::Functions::Propagate(r_ch_Eci_km, v_ch_Eci_kms, t_f, rChEciKm, vChEciKms);
	Ric2Eci(rChEciKm, vChEciKms, rDepEciKm, vEciKms, minTargetKm, v2);
	Ric2Eci(rChEciKm, vChEciKms, rDepEciKm, vEciTargetKms, minTargetKm, minTargetKms);

	v2 = vEciTargetKms - vEciKms;
}

void IpoptInitialGuess(double* ipoptVariables, double* dv1, double* dv2, double tf, Objects::Asset assetObject)
{

}

void Eci2Ric(Vector3d r_chief, Vector3d dr_chief, Vector3d r_deputy, Vector3d dr_deputy, Vector3d& r_HCW, Vector3d& dr_HCW)
{
	/*
	Algorithm converts a deputy state in the inertial frame to a state in the relative frame
	*/
	
	Vector3d h, rci_X, rci_Xd, rci_Y, rci_Yd, rci_Z, rci_Zd;
	Matrix3d C, Cd;
	h = r_chief.cross(dr_chief);

	rci_X = r_chief / r_chief.norm();
	rci_Z = h / h.norm();
	rci_Y = rci_Z.cross(rci_X);

	rci_Xd = (1 / r_chief.norm())*(dr_chief - rci_X.dot(dr_chief)*rci_X);
	rci_Yd = rci_Z.cross(rci_Xd);
	rci_Zd << 0, 0, 0;

	C << rci_X(0), rci_X(1), rci_X(2),
		rci_Y(0), rci_Y(1), rci_Y(2),
		rci_Z(0), rci_Z(1), rci_Z(2);
	Cd << rci_Xd(0), rci_Xd(1), rci_Xd(2),
		rci_Yd(0), rci_Yd(1), rci_Yd(2),
		rci_Zd(0), rci_Zd(1), rci_Zd(2);
	//cout << "C\n" << C << "\nCd\n" << Cd << endl;
	r_HCW = C*(r_deputy - r_chief);
	dr_HCW = Cd*(r_deputy - r_chief) + C*(dr_deputy - dr_chief);

}

void Ric2Eci(Vector3d r_chief, Vector3d dr_chief, Vector3d& r_deputy, Vector3d& dr_deputy, Vector3d r_HCW, Vector3d dr_HCW)
{
	/*
	Algorithm converts a deputy state in the relative frame to a state in the inertial frame
	*/
	
	Vector3d h, rci_X, rci_Xd, rci_Y, rci_Yd, rci_Z, rci_Zd;
	Matrix3d C, Cd;
	h = r_chief.cross(dr_chief);

	rci_X = r_chief / r_chief.norm();
	rci_Z = h / h.norm();
	rci_Y = rci_Z.cross(rci_X);

	rci_Xd = (1 / r_chief.norm())*(dr_chief - rci_X.dot(dr_chief)*rci_X);
	rci_Zd << 0, 0, 0;
	rci_Yd = rci_Z.cross(rci_Xd);

	C << rci_X(0), rci_X(1), rci_X(2),
		rci_Y(0), rci_Y(1), rci_Y(2),
		rci_Z(0), rci_Z(1), rci_Z(2);
	Cd << rci_Xd(0), rci_Xd(1), rci_Xd(2),
		rci_Yd(0), rci_Yd(1), rci_Yd(2),
		rci_Zd(0), rci_Zd(1), rci_Zd(2);

	//cout << "C\n" << C << "\nCd\n" << Cd << endl;

	r_deputy = C.transpose()*r_HCW;
	dr_deputy = Cd.transpose()*r_HCW + C.transpose()*dr_HCW;

	r_deputy += r_chief;
	dr_deputy += dr_chief;

}

Matrix3d DCMIn2HCW(Vector3d r_chief, Vector3d dr_chief)
{
	/*
	Algorithm produces direction cosine matrix converting an inertial coordinate into an LVLH coordinate
	based on the resident space object, wherein the X-axis is along the radial vector, Y-axis is along resident velocity vector,
	and Z-axis is along the momentum vector
	*/

	Vector3d inertX, inertY, inertZ, lvlhX, lvlhY, lvlhZ, cddr;

	// Establish inertial reference frame
	inertX << 1, 0, 0;
	inertY << 0, 1, 0;
	inertZ << 0, 0, 1;

	// Establish LVLH frame for HCW calculations
	lvlhX = r_chief / r_chief.norm();
	cddr = r_chief.cross(dr_chief);
	lvlhZ = cddr / cddr.norm();
	lvlhY = lvlhZ.cross(lvlhX);

	Matrix3d D;
	D << lvlhX.dot(inertX), lvlhX.dot(inertY), lvlhX.dot(inertZ),
		lvlhY.dot(inertX), lvlhY.dot(inertY), lvlhY.dot(inertZ),
		lvlhZ.dot(inertX), lvlhZ.dot(inertY), lvlhZ.dot(inertZ);
	return D;
}

void HcwStateTranstion(double n, double t, Matrix3d& M, Matrix3d& Ninv, Matrix3d& S, Matrix3d& T)
{
	M << 4 - 3 * cos(n*t), 0, 0,
		6 * (sin(n*t) - n*t), 1, 0,
		0, 0, cos(n*t);

	double bottom = 4 * pow(cos(n*t), 2) - 8 * cos(n*t) + 4 * pow(sin(n*t), 2) - 3 * n*t*sin(n*t) + 4;
	Ninv << n*(4 * sin(n*t) - 3 * n*t) / bottom, 2 * n*(cos(n*t) - 1) / bottom, 0,
		-2 * n*(cos(n*t) - 1) / bottom, n*sin(n*t) / bottom, 0,
		0, 0, n / sin(n*t);

	S << 3 * n * sin(n*t), 0, 0,
		6 * n * (cos(n*t) - 1), 0, 0,
		0, 0, -n * sin(n*t);

	T << cos(n*t), 2 * sin(n*t), 0,
		-2 * sin(n*t), 4 * cos(n*t) - 3, 0,
		0, 0, cos(n*t);
}

Vector3d SunVector(double JD_UTI, Vector3d& r_sun)
{
	/*
	Tested 18 Jan 18
	
	Source: Vallado
	*/
	
	double T_UTI, lambda_M, M, lambda_eliptic, r, epsilon;

	T_UTI = (JD_UTI - 2451545) / 36525;
	lambda_M = 280.46 + 36000.77*T_UTI;

	M = 357.5277233 + 35999.05034*T_UTI;
	lambda_eliptic = lambda_M + 1.914666471*sin(M*pi / 180) + 0.019994643*sin(2 * M*pi / 180);
	r = 1.000140612 - 0.016708617*cos(M*pi / 180) - 0.000139589*cos(2 * M*pi / 180);
	epsilon = 23.439291 - 0.0130042*T_UTI;

	r_sun << r*cos(lambda_eliptic*pi / 180),
			 r*cos(epsilon*pi / 180)*sin(lambda_eliptic*pi / 180),
			 r*sin(epsilon*pi / 180)*sin(lambda_eliptic*pi / 180);
	return r_sun;
}

void HcwTargeting(Vector3d HCW_r0, Vector3d HCW_rf,  double t,
	Vector3d& v0, Vector3d& vf, double n_chief)
{
	Matrix3d M, Ninv, S, T;

	HcwStateTranstion(n_chief, t, M, Ninv, S, T);\
	v0 = Ninv*(HCW_rf - M*HCW_r0);
	vf = S*HCW_r0 + T*v0;
	//cout << "HCW Targeting v0\n" << v0 << "\nvf\n" << vf << endl;
}

void NmcCalculator(double ae, double B, double gamma, double zmax, double n, Vector3d& r, Vector3d& dr)
{
	r << -ae / 2 * cos(B), ae*sin(B), zmax*sin(gamma + B);
	dr << ae / 2 * n*sin(B), ae*n*cos(B), zmax*n*cos(gamma + B);
}

void Teardrop_Calculator(double n, double D, double Tp, double B, Vector3d& r, Vector3d& dr)
{
	double gamma = n*Tp / 2;
	double s_gamma = sin(gamma);
	double ae = abs(6 * D*gamma / (3 * gamma - 4 * s_gamma));
	double xd = -4 * D*s_gamma / (3 * gamma - 4 * s_gamma);
	double yd0 = 3 / 2 * n*xd*(pi / n);


	double t_beta;
	if (D < 0)
	{
		t_beta = B / n;
	}
	else
	{
		t_beta = (B - pi) / n;
	}

	double yd = yd0 - 3 / 2 * n*xd*t_beta;

	r << -ae / 2 * cos(B) + xd, ae*sin(B) - yd, 0;
	
	dr << ae / 2 * n*sin(B), ae*n*cos(B) - 3 / 2 * n*xd, 0;
}

void ProxOps::Functions::ProxOpsMinimization(double* r_dep_Eci_km, double* v_dep_Eci_kms, double* r_ch_Eci_km, double* v_ch_Eci_kms, bool sunConstraint, 
	double time_mjd, rpoType type, double* parameters, double* dv1, double* dv2, double& t_f, bool ipopt)
{
	/*
		Function designed to find a transfer time approaching a minimum into either a teardrop or NMC RPO maneuver. Uses HCW targeting to approximate the maneuver 
		utilizing impulsive calculations. For NMC, paramters are [0] ae, [1] gamma, [2] zmax, [3] max transfer time. For teardrop, parameters are [0] D, [1] Tp. 

		Function Inputs:
			dep_Eci --> Deputy state in ECI frame, assumed to be current at start of maneuver
			ch_Eci --> Chief state in ECI frame, assumed to be current at start of maneuver
		 	parameters --> describes trajectory as explained in paragraph above
			type --> Either NMC or teardrop
			time_mjd --> time at start of maneuver
			sunConstaint --> boolean deciding whether algorithm attempts to match sun vector at completion of maneuver
		
		Function outputs dv1, dv2, and t_f. dv1 & dv2 are thrust vectors in the ECI frame, t_f is transfer time of maneuver.
	*/
	double min, minT, n, dV;
	Vector3d r_sun, rChEciKm, vChEciKms, rDepEciKm, vDepEciKms, rTargetKm, vTargetKms, 
		rDepHcwKm, vDepHcwKms, minV1, minV2, v1, v2, vEciKms, vEciTargetKms, minTargetKm, minTargetKms;
	Matrix3d D;

	// Put chief and deputy states into Vector3d vectors
	for (int kk = 0; kk < 3; kk++)
	{
		rChEciKm(kk) = r_ch_Eci_km[kk];
		vChEciKms(kk) = v_ch_Eci_kms[kk];
		rDepEciKm(kk) = r_dep_Eci_km[kk];
		vDepEciKms(kk) = v_dep_Eci_kms[kk];
	}

	n = meanMotionCalc(rChEciKm, vChEciKms);

	// Set initial parameters for search space
	min = 1000000;
	minT = 0;
	

	// Find deputy state in relative RIC frame
	Eci2Ric(rChEciKm, vChEciKms, rDepEciKm, vDepEciKms, rDepHcwKm, vDepHcwKms);

	//cout << "HCW State\n" << rDepHcwKm << endl << vDepHcwKms << endl;

	if (type == NMC)
	{
		double t_low, t_high, t_step, t_min, t_max, beta;
		t_low = 1000;
		t_high = parameters[3];
		t_max = t_high;
		t_min = t_low;
		t_step = 1000;
		for (int iter = 0; iter < 10; iter++)
		{
			for (double t = t_low; t <= t_high; t += t_step)
			{
				if (sunConstraint)
				{
					// Calculate sun vector
					SunVector(time_mjd + t / 86400, r_sun);
					//std::cout << "Sun Vector\n" << r_sun << std::endl;

					//Propagate chief to state of NMC entry
					Propagator::Functions::Propagate(r_ch_Eci_km, v_ch_Eci_kms, t, rChEciKm, vChEciKms);
					//std::cout << "Chief r\n" << rChEciKm << endl << "Chief v\n" << vChEciKms << endl;

					// Construct DCM for chief position to translate sun vector to relative frame
					D = DCMIn2HCW(rChEciKm, vChEciKms);
					r_sun = D*r_sun;

					// Find beta angle coinciding with sun vector
					beta = atan2(r_sun(1), -2 * r_sun(0));

					NmcCalculator(parameters[0], beta, parameters[1], parameters[2], n, rTargetKm, vTargetKms);
					//cout << "R Target\n" << rTargetKm << "\n V Target\n" << vTargetKms << "\n R Dep\n" << rDepHcwKm << "\nV Dep\n" << vDepHcwKms << endl;
					HcwTargeting(rDepHcwKm, rTargetKm, t, v1, v2, n);
					//cout << "v1\n" << v1 << "\nv2\n" << v2 << endl;
					dV = (v1 - vDepHcwKms).norm() + (vTargetKms - v2).norm();
					//cout << "dV = " << dV << endl;
					//cout << "Transfer Time = \n" << t << endl;
					//system("pause");
					if (dV < min) // IF delta V less than current minimum delta V, replace all values with current values
					{
						min = dV;
						//std::cout << "New min: " << min << " m/s\n Transfer Time: " << t << " s\n";
						minT = t;
						t_f = t;
						minTargetKm = rTargetKm;
						minTargetKms = vTargetKms;
						minV1 = v1;
						minV2 = v2;
					}

				}
			}
			t_low = minT - 2 * t_step;
			t_high = minT + 2 * t_step;
			if (t_high > t_max)
			{
				t_high = t_max;
			}
			if (t_low < t_min)
			{
				t_low = t_min;
			}
			t_step = t_step / 2;
		}

	}
	else if (type == Teardrop)
	{
		if (sunConstraint)
		{
			double betaSun, betaMin, betaMax, t_sun, t_man;
			if (parameters[0] < 0)
			{
				betaSun = pi;
				betaMin = pi / 2;
				betaMax = 3 * pi / 4;
			}
			else
			{
				betaSun = 2 * pi;
				betaMin = 3 * pi / 2;
				betaMax = 7 * pi / 4;
			}

			for (double t = 0; t < 86400; t++)
			{
				SunVector(time_mjd + t / 86400, r_sun);
				Propagator::Functions::Propagate(r_ch_Eci_km, v_ch_Eci_kms, t, rChEciKm, vChEciKms);
				r_sun = DCMIn2HCW(rChEciKm, vChEciKms)*r_sun;
				if (parameters[0] * r_sun(0) > 0)
				{
					if (abs(r_sun(1)) < 0, 01)
					{
						t_sun = t;
					}
				}
			}

			for (double b = betaMin; b <= betaMax; b += 0.01)
			{
				t_man = t_sun - (betaSun - b) / n;
				Teardrop_Calculator(n, parameters[0], parameters[1], b, rTargetKm, vTargetKms);
				HcwTargeting(rDepHcwKm, rTargetKm, t_man, v1, v2,n);
				dV = (v1 - vDepHcwKms).norm() + (v2 - vTargetKms).norm();
				if (dV < min)
				{
					min = dV;
					minT = t_man;
					t_f = t_man;
					minV1 = v1;
					minV2 = v2;
					minTargetKm = rTargetKm;
					minTargetKms = vTargetKms;
				}
			}
		}
		else
		{
			double betaMin, betaMax, t_man, tLow, tHigh, tStep, tMax;
			if (parameters[0] < 0)
			{
				betaMin = pi / 2;
				betaMax = 3 * pi / 4;
			}
			else
			{
				betaMin = 3 * pi / 2;
				betaMax = 7 * pi / 4;
			}

			for (double beta = betaMin; beta <= betaMax; beta += 0.01)
			{
				tLow = 1e3;
				tHigh = 3600;
				tStep = 900;
				tMax = tHigh;
				for (int kk = 0; kk < 10; kk++)
				{
					Teardrop_Calculator(n, parameters[0], parameters[1], b, rTargetKm, vTargetKms);
					HcwTargeting(rDepHcwKm, rTargetKm, t_man, v1, v2, n);
					dV = (v1 - vDepHcwKms).norm() + (v2 - vTargetKms).norm();
					if (dV < min)
					{
						min = dV;
						minT = t_man;
						t_f = t_man;
						minV1 = v1;
						minV2 = v2;
						minTargetKm = rTargetKm;
						minTargetKms = vTargetKms;
					}
					tLow = minT - 2 * tStep;
					tHigh = minT + 2 * tStep;
					if (tHigh > tMax)
					{
						tHigh = tMax;
					}
					tStep /= 2;
				}
			}
		}
	}

	if (ipopt)
	{
		// If using IPOPT solver, return to program

		Vector3d dv1vector, dv2vector;
		dv1vector = v1 - vDepEciKms;
		dv2vector = vTargetKms - v2;

		for (int kk = 0; kk < 3; kk++)
		{
			dv1[kk] = dv1vector(kk);
			dv2[kk] = dv2vector(kk);
		}

		return;
	}
	
	
	
	v1 = minV1;
	v2 = minV2;
	//cout << "v1 HCW\n" << v1 << "\nv2 HCW\n" << v2 << endl;

	// Revert chief state back to initial
	for (int kk = 0; kk < 3; kk++)
	{
		rChEciKm(kk) = r_ch_Eci_km[kk];
		vChEciKms(kk) = v_ch_Eci_kms[kk];
	}

	// Convert everything back to inertial to calculate inertial delta-V requirements
	Ric2Eci(rChEciKm, vChEciKms, rDepEciKm, vEciKms, rDepHcwKm, v1);
	//cout << "vEciKms \n" << vEciKms << endl;
	v1 = vEciKms - vDepEciKms;

	// Propagate chief to final burn and calculate second dV
	Propagator::Functions::Propagate(r_ch_Eci_km, v_ch_Eci_kms, t_f, rChEciKm, vChEciKms);
	Ric2Eci(rChEciKm, vChEciKms, rDepEciKm, vEciKms, minTargetKm, v2);
	Ric2Eci(rChEciKm, vChEciKms, rDepEciKm, vEciTargetKms, minTargetKm, minTargetKms);

	v2 = vEciTargetKms - vEciKms;
	//cout << "v1 ECI\n" << v1 << "\nv2 ECI\n" << v2 << endl;


	// Move delta-V into double form for output
	for (int kk = 0; kk < 3; kk++)
	{
		dv1[kk] = v1(kk);
		dv2[kk] = v2(kk);

	}
}

void ProxOps::Functions::PromityOperations(Objects::Asset assetObject, Objects::Target targetObject, double simTime_mjd, 
	Objects::satCmdMessage* outMessages)
{
	// Propagate asset and target to the same epoch (simTime + 1 hour)
	bool ipopt = false; // Change to true if using IPOPT
	double *parameters, t_f;
	if (targetObject.manType = NMC)
	{
		parameters		= new double[4]; 
		parameters[0]	= *targetObject.ae_km;
		parameters[1]	= *targetObject.zmax_km;
		parameters[2]	= *targetObject.gamma_rad;
		parameters[3]	= 10000;
	}
	else if (targetObject.manType == Teardrop)
	{
		parameters		= new double[3];
		parameters[0]	= *targetObject.Tp_s;
		parameters[1]	= *targetObject.D_km;
		parameters[2]	= 10000;
	}
	else
	{
		parameters = new double;
	}
	double *dv1 = new double[3];
	double *dv2 = new double[3];


	ProxOpsMinimization(assetObject.oscState_pos_gcrf_km, assetObject.oscState_vel_gcrf_kms, targetObject.oscState_pos_gcrf_km, targetObject.oscState_vel_gcrf_kms,
		targetObject.sunConstraint, simTime_mjd, targetObject.manType, parameters, dv1, dv2, t_f, ipopt);
	
	if (ipopt)
	{
		// Call initial guess generator
		double* ipoptVariables = new double[7];
		IpoptInitialGuess(ipoptVariables, dv1, dv2, t_f, assetObject);

		// Call IPOPT DLL wrapper

		// Convert back to inertial commands
	}
	
	
	
	// Create outgoing messages
	outMessages = new Objects::satCmdMessage[2];
	double norm = sqrt(dv1[0] * dv1[0] + dv1[1] * dv1[1] + dv1[2] * dv1[2]);
	int kk = 0;
	outMessages[kk].durationEstimate_s = BurnEstimator(assetObject.dryMass_kg, assetObject.fuelRemaining_kg,
		assetObject.massRate_kgs, assetObject.Isp_s, norm);
	outMessages[kk].startTime_mjd = simTime_mjd + 0.0416667 - outMessages[kk].durationEstimate_s / 2;
	for (int jj = 0; jj < 3; jj++)
	{
		outMessages[kk].thrustVector_kms[jj] = dv1[jj];
	}
	outMessages[kk].referenceFrame = gcrf;
	
	
	
	kk = 1;
	norm = sqrt(dv2[0] * dv2[0] + dv2[1] * dv2[1] + dv2[2] * dv2[2]);
	outMessages[kk].durationEstimate_s = BurnEstimator(assetObject.dryMass_kg, assetObject.fuelRemaining_kg,
		assetObject.massRate_kgs, assetObject.Isp_s, norm);
	outMessages[kk].startTime_mjd = simTime_mjd + 0.0416667 - outMessages[kk].durationEstimate_s / 2;
	for (int jj = 0; jj < 3; jj++)
	{
		outMessages[kk].thrustVector_kms[jj] = dv2[jj];
	}
	outMessages[kk].referenceFrame = gcrf;

	delete[] parameters;
	delete[] dv1;
	delete[] dv2;
}
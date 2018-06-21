#pragma once

#include <iostream>

enum ackResponse {WILCO, CANTCO, HAVCO, NA};
enum cmdType {New, Update, Cancel};
enum rpoType {Teardrop, NMC, Hover};
enum state {Idle, Burn1, Burn2, Coast, RPO};
enum frame {gcrf, j2000, hcw};
enum satType {A,B,C,D};


#ifdef OBJECT_EXPORTS
#define OBJECT_API __declspec(dllexport)
#else
#define OBJECT_API __declspec(dllimport)
#endif


namespace Objects
{
	
	class __declspec(dllexport) Satellite
	{
	public:
		double oscState_pos_gcrf_km[3];
		double oscState_vel_gcrf_kms[3];
		double epoch_mjd;
		int ID;
	};
	
	
	
	class __declspec(dllexport) Target : public Satellite
	{
	public:
		Target() 
		{

		}

		~Target()
		{

		}
		
		
		double manWindow_mjd[2];
		satType reqType;
		rpoType manType;
		bool sunConstraint;
		double maxFuelPercent; // decimal value percentage of maximum fuel comsumption allowed
		
		// NMC Parameters
		double* ae_km;
		double* zmax_km;
		double* gamma_rad;
		
		// Teardrop Parametesr
		double* D_km;
		double* Tp_s;
		
	};

	class __declspec(dllexport) satCmdMessage
	{
	public:
		satCmdMessage() 
		{
			this->type = New;
		}
		~satCmdMessage()
		{
			std::cout << "Cmd Message deleted\n";
		}
	

		int 		ID;
		cmdType 	type;
		double 		startTime_mjd;
		double	 	thrustVector_kms[3];
		double		durationEstimate_s;
		frame 		referenceFrame;

	};

	struct stateMachine
	{
	public:
		stateMachine() {}
		~stateMachine() {}


		double 			coastStart_mjd;
		double 			coastTime_mjd;
		Target 			target;
		int 			c2CommandID;
		satCmdMessage	lastCommand;

		double 			messageTime_mjd;
		double 			numAttempts_nd;
		bool 			awaitingAck;
	};

	class __declspec(dllexport) Asset : public Satellite
	{
	public:
		Asset() : machineState(Idle)
		{

		}
		~Asset()
		{

		}

		state machineState;

		stateMachine* stateMachineObject;

		// Propulsion information
		double Isp_s;
		double engineForce_N;
		double dryMass_kg;
		double fuelRemaining_kg;
		double massRate_kgs;

	};



	
	
	class __declspec(dllexport) c2CmdMessage
	{
	public:
		c2CmdMessage()
		{
			
		}
		cmdType 	type;
		int 		ID;
		int 		assetID;
		Target 		targetObject;
		
		
	};
	
	class __declspec(dllexport) ackMessage
	{
	public:
		ackMessage() {}
		~ackMessage()
		{
			std::cout << "Ack Message deleted\n";
		}
		ackResponse 	response;
		int 			commandID;
	};
	
	class __declspec(dllexport) Messages
	{
	public:
		Messages() : numC2Cmds(0), numSatCmds(0), numAcks(0) 
		{
			c2Cmds = new c2CmdMessage;
			satCmds = new satCmdMessage;
			ackMsgs = new ackMessage;
		};
		
		int 			numC2Cmds;
		c2CmdMessage 	*c2Cmds;
		int 			numSatCmds;
		satCmdMessage 	*satCmds;
		int 			numAcks;
		ackMessage 		*ackMsgs;
		
	};

}
/***
@Chunting
In this version, I put all the result data into a single file, Result.dat
**/
#include "Thermal_Bus31.h"

int main(int argc, char *argv[])
{
	IloEnv env;
	int t,i,j,k,l,d,day;
	try
	{
		IloModel model(env);
		IloTimer timer(env);
		timer.start();
		//*************读取系统变量***************************	
		readSystemData(SYSTEMDATA,
			env,
			cycle,			//时段个数
			thUnitNum,		//火电机组个数
			demandNum,		//负荷个数
			lineNum,			//传输线个数
			busNum,			//节点个数
			horizon
			);
		ifstream fin(APPDATA,ios::in);
		if(!fin) 
			env.out()<<"problem with file:"<<APPDATA<<endl;

		IloNumArray sysDemand(env,cycle+1);
		IloNumArray sysReserve(env,cycle+1);
		day=1;
		//sysDemand
		for(t=1;t<cycle/day+1;t++)
			fin>>sysDemand[t];
		// Here, what does the day mean?
		for (d=1;d<day;d++)
		{
			for (t=cycle/day*d+1;t<cycle/day*(d+1)+1;t++)
			{
				sysDemand[t]=sysDemand[t-cycle/day];
			}
		}
		//sysReserve
		for(t=1;t<cycle/day+1;t++)
			fin>>sysReserve[t];
		for (d=1;d<day;d++)
		{
			for (t=cycle/day*d+1;t<cycle/day*(d+1)+1;t++)
			{
				sysReserve[t]=sysReserve[t-cycle/day];
			}
		}


		//*************读取网络数据，王圭070411**************************		
		IloNumArray unitLocation(env,thUnitNum);
		IloNumArray demandLocation(env,demandNum);
		IloNumArray demand(env,demandNum);
		/*
		//Matrix2 demand(env,demandNum);//二维数组
		for(i=0;i<demandNum;i++)
		demand[i]= IloNumArray(env,cycle+1);
		*/
		IloNumArray lineCap(env,lineNum);

		readNetData(NETDATA,env, 
			unitLocation,              //机组所在bus编号
			demandLocation,            //负载所在bus编号
			demand,                    //各负载需求
			lineCap
			);





		//*************定义机组变量**************************
		IloNumArray thminPower(env,thUnitNum);                                    //最小发电量
		IloNumArray thmaxPower(env,thUnitNum);                                    //最大发电量
		IloNumArray thminFuelCost(env,thUnitNum);
		IloNumArray thmaxFuelCost(env,thUnitNum);
		IloNumArray thminUp(env,thUnitNum);                                       //最小开机时间
		IloNumArray thminDown(env,thUnitNum);                                     //最小关机时间
		IloNumArray thcoldUpTime(env,thUnitNum);
		IloNumArray thfuelCostPieceNum(env,thUnitNum);
		IloNumArray thhotUpCost(env,thUnitNum);
		IloNumArray thcoldUpCost(env,thUnitNum);
		IloNumArray thdelta(env,thUnitNum);			
		//IloNumArray thfirstLast(env,thUnitNum);
		IloNumArray thmaxR(env,thUnitNum);
		IloNumArray tha(env,thUnitNum);
		IloNumArray thb(env,thUnitNum);
		IloNumArray thc(env,thUnitNum);
		IloNumArray thinitState(env,thUnitNum);
		IloNumArray thinitPower(env,thUnitNum);



		//*************读火电取机组数据***************************
		readThUnitData(THUNITDATA,env,
			thminPower,                        //机组最小发电量
			thmaxPower,                        //机组最大发电量
			thminDown,                         //机组最小关机时间
			thminUp,                           //机组最小开机时间
			thcoldUpTime,                      //冷启动时间
			thfuelCostPieceNum,                //燃料费用曲线段数
			thhotUpCost,                       //热启动费用
			thcoldUpCost,			           //冷启动费用
			thdelta,						      //爬升
			//	thfirstLast,                       //首末开机约束，取0，1
			thmaxR,                          //机组最大备用	
			tha,							     //燃料费用曲线上系数a		
			thb,							     //燃料费用曲线上系数b		
			thc,
			thminFuelCost,
			thmaxFuelCost,
			thinitState,
			thinitPower
			);
		//******************读取火电分段线性数据*************
		Matrix2      thfuelCostPieceSlope(env,thUnitNum);
		for(i=0;i<thUnitNum;i++)
		{
			thfuelCostPieceSlope[i]=IloNumArray(env,thfuelCostPieceNum[i]);
		}

		Matrix2      thminFuelPieceCost(env,thUnitNum);
		for(i=0;i<thUnitNum;i++)
		{
			thminFuelPieceCost[i]=IloNumArray(env,thfuelCostPieceNum[i]);
		}

		Matrix2      thmaxFuelPieceCost=Matrix2(env,thUnitNum);
		for(i=0;i<thUnitNum;i++)
		{
			thmaxFuelPieceCost[i]=IloNumArray(env,thfuelCostPieceNum[i]);
		}

		Matrix2     thminPiecePower(env,thUnitNum);
		for(i=0;i<thUnitNum;i++)
		{
			thminPiecePower[i]=IloNumArray(env,thfuelCostPieceNum[i]);
		}

		Matrix2      thmaxPiecePower(env,thUnitNum);
		for(i=0;i<thUnitNum;i++)
		{
			thmaxPiecePower[i]=IloNumArray(env,thfuelCostPieceNum[i]);
		}
		pieceThUnitData(env,
			tha,							   //燃料费用曲线上系数a		
			thb,							   //燃料费用曲线上系数b		
			thc,							   //燃料费用曲线上系数b
			thminPower,                        //机组最小发电量
			thmaxPower,                        //机组最大发电量
			thfuelCostPieceNum,
			thminPiecePower,
			thmaxPiecePower,
			thminFuelPieceCost,
			thmaxFuelPieceCost,
			thfuelCostPieceSlope
			); 

		//************读取gama*********
		Matrix2      gama(env, lineNum);
		for(i=0;i<lineNum;i++)
		{
			gama[i]=IloNumArray(env, busNum);
		}
		readGama(GAMADATA,env,gama);

		//输出
		ofstream tfile(OUTFILEDATA,ios::out);
		if(!tfile)
			cout<<"cannot open "<<OUTFILEDATA<<endl;
		tfile<<"cycle="<<cycle<<endl;

		tfile<<"thUnitNum="<<thUnitNum<<endl;

		tfile<<"lineNum="<<lineNum<<endl;
		tfile<<"busNum="<<busNum<<endl;
		tfile<<"demandNum="<<demandNum<<endl;

		tfile<<endl<<"***********************网络数据****************"<<endl;
		tfile<<endl<<"unit location"<<endl;
		for (i=0;i<thUnitNum;i++)
		{
			tfile<<unitLocation[i]<<"\t"; 
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}

		tfile<<endl<<"demand location"<<endl;
		for (i=0;i<demandNum;i++)
		{
			tfile<<demandLocation[i]<<"\t";
			if( (i+1)%11 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"demand***************After changed**********"<<endl;

		for (i=0;i<demandNum;i++)
		{
			tfile<<demand[i]<<"\t";
			if( (i+1)%11 == 0 && i != 0 )
				tfile << endl;
		}

		/**
		for (i=0;i<demandNum;i++)
		{
		for(t=1;t<cycle+1;t++)
		tfile<<demand[i][t]<<"\t";
		tfile<<endl;
		}
		**/	
		tfile<<endl<<"line capacity"<<endl;
		for (i=0;i<lineNum;i++)
		{
			tfile<<lineCap[i]<<"\t";
			if( (i+1)%39 == 0 && i != 0)
				tfile << endl;
		}

		tfile<<endl<<"Gama: ---------Begin-------"<<endl; 
		tfile<<"busNum "<<busNum<<" linNum "<<lineNum<<endl;
		for(j=0;j<lineNum;j++)
		{
			for(i=0;i<busNum;i++)
			{
				tfile<<gama[j][i]<<"  ";
			}
			tfile<<endl;
		}	
		tfile<<"  ---------End----"<<endl; 

		tfile<<endl<<"thminPower"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thminPower[i]<<" ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thmaxPower"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thmaxPower[i]<<" ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thminDown"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thminDown[i]<<"  ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thminUp"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thminUp[i]<<"  ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thcoldUpTime"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thcoldUpTime[i]<<"  ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thfuelCostPieceNum"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thfuelCostPieceNum[i]<<" ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thhotUpCost"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thhotUpCost[i]<<"  ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thcoldUpCost"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thcoldUpCost[i]<<" ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thdelta"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thdelta[i]<<" ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thmaxR"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thmaxR[i]<<" ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"tha"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<tha[i]<<"  ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thb"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thb[i]<<"  ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thc"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thc[i]<<"  ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thminFuelCost"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thminFuelCost[i]<<" ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thmaxFuelCost"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thmaxFuelCost[i]<<" ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thinitState"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thinitState[i]<<"\t";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}
		tfile<<endl<<"thinitPower"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			tfile<<thinitPower[i]<<" ";
			if( (i+1)%16 == 0 && i != 0)
				tfile << endl;
		}

		tfile<<endl<<"**************火电机组分段线性数据***********"<<endl;
		tfile<<"thminFuelPieceCost"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			for(j=0;j<thfuelCostPieceNum[i];j++)
			{
				tfile<<thminFuelPieceCost[i][j]<<"  ";
			}
			tfile<<endl;
		}
		tfile<<endl<<"thmaxFuelPieceCost"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			for(j=0;j<thfuelCostPieceNum[i];j++)
			{
				tfile<<thmaxFuelPieceCost[i][j]<<"  ";
			}
			tfile<<endl;
		}
		tfile<<endl<<"thminPiecePower"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			for(j=0;j<thfuelCostPieceNum[i];j++)
			{
				tfile<<thminPiecePower[i][j]<<"  ";
			}
			tfile<<endl;
		}
		tfile<<endl<<"thmaxPiecePower"<<endl;
		for(i=0;i<thUnitNum;i++)                                                        
		{
			for(j=0;j<thfuelCostPieceNum[i];j++)
			{
				tfile<<thmaxPiecePower[i][j]<<"  ";
			}
			tfile<<endl;
		}
		tfile<<endl<<"thfuelCostPieceSlope"<<endl;
		for(i=0;i<thUnitNum;i++)
		{
			for(j=0;j<thfuelCostPieceNum[i];j++)
			{
				tfile<<thfuelCostPieceSlope[i][j]<<"  ";
			}
			tfile<<endl;
		}

		tfile.close();

		//************************火电变量定义***********************
		//定义状态变量,0-1变量，取1时，表示该机组在第t时刻处于开机状态
		VarMatrix2 state(env,thUnitNum);
		for(i = 0; i < thUnitNum; i++)
		{
			state[i] = IloNumVarArray(env, cycle+1, 0, 1, ILOINT);
		}
		//火电发电量
		VarMatrix2 thermalPower(env,thUnitNum);
		for(i=0;i<thUnitNum;i++)
		{
			thermalPower[i]=IloNumVarArray(env,cycle+1,0,thmaxPower[i],ILOFLOAT);
		}


		//火电备用
		VarMatrix2 thermalR(env,thUnitNum);
		for(i=0;i<thUnitNum;i++)
		{
			thermalR[i]=IloNumVarArray(env,cycle+1,0,thmaxR[i],ILOFLOAT);
		}
		//开机操作变量
		VarMatrix2 startUp= VarMatrix2(env,thUnitNum);
		for(i = 0; i < thUnitNum; i++)
		{
			startUp[i] = IloNumVarArray(env, cycle+1, 0, 1, ILOINT);
		}
		//关机操作变量
		VarMatrix2 shutDown= VarMatrix2(env,thUnitNum);
		for(i=0;i<thUnitNum;i++)
		{
			shutDown[i]=IloNumVarArray(env,cycle+1,0,1,ILOINT);	
		}
		//启动费用
		VarMatrix2 upCost(env,thUnitNum);
		for(i=0;i<thUnitNum;i++)
		{
			upCost[i]=IloNumVarArray(env,cycle+1,0,thcoldUpCost[i],ILOFLOAT);
		}
		//燃料费用
		VarMatrix2 fuelCost=VarMatrix2(env,thUnitNum);
		for(i=0;i<thUnitNum;i++)
		{
			fuelCost[i]=IloNumVarArray(env,cycle+1,0,thmaxFuelCost[i],ILOFLOAT);
		}

		//************与分段线性有关的变量*************************
		//分段线性状态
		VarMatrix3 pieceState = VarMatrix3(env, thUnitNum);
		for(i = 0; i < thUnitNum; i++)
		{
			pieceState[i] = VarMatrix2(env, cycle+1);
			for(t = 0; t < cycle+1; t++)
			{
				pieceState[i][t] = IloNumVarArray(env, thfuelCostPieceNum[i], 0, 1, ILOINT);
			}
		}
		//发电量
		VarMatrix3 thermalPiecePower=VarMatrix3(env,thUnitNum);
		for(i=0;i<thUnitNum;i++)
		{
			thermalPiecePower[i]=VarMatrix2(env,cycle+1);
			for(t=0;t<cycle+1;t++)
			{
				thermalPiecePower[i][t]=IloNumVarArray(env,thfuelCostPieceNum[i],0,thmaxPower[i],ILOFLOAT);
			}
		}
		//分段线性部分上的燃料费用
		VarMatrix3 fuelPieceCost=VarMatrix3(env,thUnitNum);
		for(i=0;i<thUnitNum;i++)
		{	
			fuelPieceCost[i]=VarMatrix2(env,cycle+1);
			for(t=0;t<cycle+1;t++)
			{
				fuelPieceCost[i][t]=IloNumVarArray(env,thfuelCostPieceNum[i],0,thmaxFuelCost[i],ILOFLOAT);
			}
		}
		//可行解约束
		for(t=1;t<cycle+1;t++)
		{
			IloExpr thsummaxp(env);
			for(i=0;i<thUnitNum;i++)
			{
				thsummaxp+=thmaxPower[i]*state[i][t];
			}
			model.add(thsummaxp >= sysDemand[t]+sysReserve[t]);
		}
		//可行解中的一个约束
		for(t=1;t<cycle+1;t++)
		{
			IloExpr thsumminp(env);

			for(i=0;i<thUnitNum;i++)
			{
				thsumminp += thminPower[i]*state[i][t];
			}
		//	model.add(thsumminp <= sysDemand[t]);
		}

		//***********************火电机组约束***********************
		//初始状态约束
		/*
		for(i = 0; i < thUnitNum; i++)
		{
			if(thinitState[i] < 0)
			{
				model.add(state[i][0] == 0);
				model.add(thermalPower[i][0] == 0);
				model.add(thermalR[i][0]==0);
			}
			else if(thinitState[i] > 0)
			{
				model.add(state[i][0] == 1);								
				model.add(thermalR[i][0]==0);
			}
		}

		//初始开关机约束
		for(i = 0; i< thUnitNum; i++)
		{
			model.add(startUp[i][0] == 0);
			model.add(shutDown[i][0] == 0);		
		}
*/
		//开关机状态约束（即状态转移约束）
		for(i = 0; i < thUnitNum; i++)
		{
			for(t = 1; t < cycle+1; t++)
			{
		//		model.add(state[i][t]-state[i][t-1]-startUp[i][t]+shutDown[i][t]==0);
		//		model.add(startUp[i][t] + shutDown[i][t]<=1);
			}
		}

		//最小开关机时间约束
		for(i = 0; i < thUnitNum; i++)
		{
			for(t = 1; t < cycle+1; t++) //从1时刻到cycle
			{
				IloNum temp1 = IloMin(cycle, t+thminUp[i]-1);//the minimum of an array of numeric expressions or over a numeric expression    and a constant in C++
				IloNum temp2 = IloMin(cycle, t+thminDown[i]-1);
				IloExpr sum1(env);
				IloExpr sum2(env);
				for(k = t+1; k <= temp1; k++)
				{
					sum1 += shutDown[i][k];
				}
				for(k = t+1; k <= temp2; k++)
				{
					sum2 += startUp[i][k];
				}
		//		model.add(startUp[i][t] + sum1 <= 1);
		//		model.add(shutDown[i][t] + sum2 <= 1);
			}
		}

		//初始开关机时间约束
		for(i=0;i<thUnitNum;i++)
		{
			if(thinitState[i]<0&&IloAbs(thinitState[i])<thminDown[i])
			{
				for(t=1;t<=(thminDown[i]+thinitState[i]);t++)
				{
					model.add(startUp[i][t]==0);
					model.add(state[i][t]==0);
				}
			}
			if(thinitState[i]>0&&thinitState[i]<thminUp[i])
			{
				for(t=1;t<=(thminUp[i]-thinitState[i]);t++)
				{
					model.add(shutDown[i][t]==0);
					model.add(state[i][t]==1);
				}
			}
		}
		//开关机操作约束（从1时刻开始）
		for(i=0;i<thUnitNum;i++)
		{
			for(t=1;t<cycle+1;t++)
			{
				IloNum temp=IloMin(cycle,(t+thminUp[i]+thminDown[i]-1));
				IloExpr sum1(env);
				IloExpr sum2(env);
				for(j=t;j<=temp;j++)
				{
					sum1+=startUp[i][j];
				}
				for(j=t;j<=temp;j++)
				{
					sum2+=shutDown[i][j];
				}
				model.add(sum1-1<=0);
				model.add(sum2-1<=0);
			}
		}

		//燃料费用曲线分段后的约束			
		for(i=0;i<thUnitNum;i++)
		{
			for(t=1;t<cycle+1;t++)
			{
				for(j=0;j<thfuelCostPieceNum[i];j++)
				{					
				//	model.add(thermalPiecePower[i][t][j]<=thmaxPiecePower[i][j]*pieceState[i][t][j]);
				//	model.add(thermalPiecePower[i][t][j]>=thminPiecePower[i][j]*pieceState[i][t][j]);
				//	model.add(fuelPieceCost[i][t][j]<=thmaxFuelPieceCost[i][j]*pieceState[i][t][j]);
				//	model.add(fuelPieceCost[i][t][j]>=thminFuelPieceCost[i][j]*pieceState[i][t][j]);
					//model.add(fuelPieceCost[i][t][j]==thminFuelPieceCost[i][j]*pieceState[i][t][j]+thfuelCostPieceSlope[i][j]*(thermalPiecePower[i][t][j]-thminPiecePower[i][j]*pieceState[i][t][j]));
					//修改by xjlei
					model.add(fuelPieceCost[i][t][j]==thminFuelPieceCost[i][j]*pieceState[i][t][j]+(thmaxFuelPieceCost[i][j]-thminFuelPieceCost[i][j])/(thmaxPiecePower[i][j]-thminPiecePower[i][j])*(thermalPiecePower[i][t][j]-thminPiecePower[i][j]*pieceState[i][t][j])-tha[i]/8*(thmaxPiecePower[i][j]-thminPiecePower[i][j])*(thmaxPiecePower[i][j]-thminPiecePower[i][j])*pieceState[i][t][j]);
				}
			}
		}
		
		//发电状态只能同时处于分段上的一段
		for(i=0;i<thUnitNum;i++)
		{
			for(t=1;t<cycle+1;t++)
			{
				IloExpr sum(env);
				for(j=0;j<thfuelCostPieceNum[i];j++)
				{
					sum +=pieceState[i][t][j];
				}
				model.add(startUp[i][t]-sum<=0);
				model.add(sum-1+shutDown[i][t]<=0);
				model.add(sum-state[i][t]==0);
			}
		}
		//发电量约束
		for(i=0;i<thUnitNum;i++)
		{
			for(t=1;t<cycle+1;t++)
			{
				IloExpr sum(env);
				for(j=0;j<thfuelCostPieceNum[i];j++)
				{
					sum+=thermalPiecePower[i][t][j];
				}
				model.add(thermalPower[i][t] == sum);
			}
		}
		
		//the constraint of fuel cost
		for(i=0;i<thUnitNum;i++)
		{
			for(t=1;t<cycle+1;t++)
			{
				IloExpr sum(env);
				for(j=0;j<thfuelCostPieceNum[i];j++)
				{
					sum += fuelPieceCost[i][t][j];
				}
				model.add(fuelCost[i][t] == sum);
			}
		}

		//备用
		for(i=0;i<thUnitNum;i++)
		{
			for(t=1;t<cycle+1;t++)
			{
				model.add(thermalR[i][t]==IloMin(thmaxPower[i]*state[i][t]-thermalPower[i][t],thmaxR[i]*state[i][t]));
			}
		}

		//启动费用约束	  
		for(i=0;i<thUnitNum;i++)
		{
			for(t=1;t<cycle+1;t++)
			{
		//		model.add(upCost[i][t]<=thcoldUpCost[i]*startUp[i][t]);
		//		model.add(upCost[i][t]>=thhotUpCost[i]*startUp[i][t]);
			}
		}
		//********************add by hx,爬升约束***********************
		for (i=0;i<thUnitNum; i++)
		{
			for (t=1; t<cycle+1;t++)
			{
				model.add(IloAbs(thermalPower[i][t]-thermalPower[i][t-1])<=thdelta[i]*horizon);
			}

		}

		//发电量约束
		for(t=1;t<cycle+1;t++)
		{
			IloExpr thsum(env);
			for(i=0;i<thUnitNum;i++)
			{
				thsum+=thermalPower[i][t];
			}
			model.add(thsum == sysDemand[t]);
		}

		//备用约束
		for(t=1;t<cycle+1;t++)
		{
			IloExpr thsum(env);
			for(i=0;i<thUnitNum;i++)
			{
				thsum+=thermalR[i][t];
			}
			model.add(thsum >= sysReserve[t]);
		}

		//传输约束
		for (t = 1; t < cycle+1; t++)
		{				 		 
			for(l= 0; l< lineNum; l++)
			{
				IloExpr sumGamaP(env);
				IloExpr sumGamaD(env);
				for (i = 0; i < thUnitNum; i++)
				{
					sumGamaP+=gama[l][(int)(unitLocation[i]-1)]*thermalPower[i][t];
				}
				for (d = 0; d < demandNum; d++)
				{
					sumGamaD+=gama[l][(int)(demandLocation[d]-1)]*sysDemand[t]*demand[d];
				} 
			//	model.add(sumGamaP-sumGamaD+lineCap[l]>=0);				 		
			//	model.add(sumGamaP-sumGamaD-lineCap[l]<=0); 
			}
		}

		//建立优化目标函数
		IloExpr obj(env);
		for(t=1;t<cycle+1;t++)
		{
			for(i=0;i<thUnitNum;i++)  
			{
				obj+=fuelCost[i][t]+upCost[i][t];
			}
		}

		IloObjective objective = IloMinimize(env, obj);
		model.add(objective);
		IloCplex cplex(model);
		cplex.setParam(cplex.EpGap,0.001);//relative MIP gap tolerance
		//	cplex.setParam(cplex.NodeFileInd,3);
		//	cplex.setParam(cplex.TiLim,100);

		cplex.extract(model);
		cplex.solve();
		cplex.exportModel(lp);//与IloCplex.importModel对应，可以读回模型

		env.out() << "Solution status = " << cplex.getStatus() << endl;
		env.out() << "Solution value  = " << cplex.getObjValue() << endl;
		env.out() << "solution time   = " <<timer.getTime()<<endl;
		env.out() << "EpGap           = " <<cplex.getParam(cplex.EpGap)<<endl;

		ofstream outf(OUTFILERESULT,ios::out);
		if(!outf)
			cout<<"cannot open 'result.dat'"<<OUTFILERESULT<<endl;

		outf<<"Result"<<endl;
		outf<<"Solution status\t"<<cplex.getStatus()<<endl;
		outf<<"Solution value\t"<<cplex.getObjValue()<<endl;
		outf<<"Solution time\t"<<timer.getTime()<<endl;
		outf<<"EpGap\t"<<cplex.getParam(cplex.EpGap)<<endl; 
		double allFuelCost=0;
		for(i=0;i<thUnitNum;i++)
		{
			for(t=1;t<cycle+1;t++)
			{
				allFuelCost+=cplex.getValue(fuelCost[i][t]);
			}
		}
		cout<<endl<<"allFuelCost="<<allFuelCost<<endl;
		outf<<"allFuelCost\t"<<allFuelCost<<endl;
		if(!outf)
			cout<<"cannot open 'Result.dat'"<<OUTFILERESULT<<endl;
		outf<<"state"<<endl;
		for(t=1;t<cycle+1;t++)
		{
			for(i=0;i<thUnitNum;i++)
			{
				if(cplex.getValue(state[i][t])<_INF)
					outf<<"0\t";
				else
					outf<<cplex.getValue(state[i][t])<<"\t";
			}
			outf<<endl;        
		}
		outf.close();
		
		ofstream outf_ThUnit(Resutl_ThUnit,ios::out);
		if(!outf_ThUnit)
			cout<<"cannot open "<<Resutl_ThUnit<<endl;
		outf_ThUnit<<endl<<"thermalPower"<<endl;
		for(t=1;t<cycle+1;t++)
		{
			for(i=0;i<thUnitNum;i++)
			{
				if(cplex.getValue(thermalPower[i][t])<_INF)
					outf_ThUnit<<"0\t";
				else
					outf_ThUnit<<cplex.getValue(thermalPower[i][t])<<"\t";
			}
			outf_ThUnit<<endl;

		}

		outf_ThUnit<<endl<<"thermalR"<<endl;
		for(t=1;t<cycle+1;t++)
		{
			for(i=0;i<thUnitNum;i++)
			{
				if(cplex.getValue(thermalR[i][t])<1e-7)
					outf_ThUnit<<"0\t";
				else
					outf_ThUnit<<cplex.getValue(thermalR[i][t])<<"\t";
			}
			outf_ThUnit<<endl;
		}
		outf_ThUnit.close();
		
		ofstream outf_Line(Result_Line,ios::out);
		if(!outf_Line)
			cout<<"cannot open "<<Result_Line<<endl;
		double current[100][200];
		for (t = 1; t < cycle+1; t++)
		{				 		 
			for(l= 0; l< lineNum; l++)
			{
				double gamap=0;
				for (i = 0; i < thUnitNum; i++)
				{
					gamap+=gama[l][(int)(unitLocation[i]-1)]*cplex.getValue(thermalPower[i][t]);
				}
				double gamaD=0;
				for (d = 0; d < demandNum; d++)
				{
					gamaD+=gama[l][(int)(demandLocation[d]-1)]*sysDemand[t]*demand[d];
				} 
				current[l][t]=gamap-gamaD;	
				outf_Line<<current[l][t]<<"\t";
			}
			outf_Line<<endl;
		}
		outf_Line<<endl;
		outf_Line.close();
	}
	catch (IloException& e) 
	{
		cerr << "Concert exception caught: " << e << endl;
	}
	catch (...) 
	{
		cerr << "Unknown exception caught" << endl;
	}

	env.end();
	system("pause"); 
	return 0;
}

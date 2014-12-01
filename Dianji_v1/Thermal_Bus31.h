#include <ilcplex/ilocplex.h>
#include <fstream>
#include <math.h>
#include <ilconcert/ilomodel.h>
/*
*/
ILOSTLBEGIN
	typedef IloArray<IloNumArray> Matrix2;  //typedefΪ IloArray<IloNumArray> �����Matrix2���Ա��ں�������ʱ����д
typedef IloArray<Matrix2> Matrix3;
typedef IloArray<Matrix3> Matrix4;
typedef IloArray<Matrix4> Matrix5;

typedef IloArray<IloNumVarArray>VarMatrix2;
typedef IloArray<VarMatrix2>VarMatrix3;
typedef IloArray<VarMatrix3>VarMatrix4;
typedef IloArray<VarMatrix4>VarMatrix5;
typedef IloArray<VarMatrix5>VarMatrix6;

//**********ˮ��������֪����������ȫ�ֱ���************
IloNum horizon;
IloInt cycle;			//��������
IloInt thUnitNum;  

//��������
IloInt demandNum;
IloInt lineNum;
IloInt busNum;

const double _INF=1E-7;//add by hx

//read ȫ����
int readSystemData(char* systemdata,
				   IloEnv env,
				   IloInt& cycle,			//ʱ�θ���  24
				   IloInt& thUnitNum,		//��������� 16
				   IloInt& demandNum,		//���ɸ���  11
				   IloInt& lineNum,			//�����߸���  39
				   IloInt& busNum,			//�ڵ����  31
				   IloNum& horizen			//ʱ�γ���  1
				   )
{

	ifstream fin(systemdata,ios::in);
	if(!fin) 
		env.out()<<"problem with file:"<<systemdata<<endl;
	fin>>cycle;
	cycle*=1;
	fin>>thUnitNum;
	fin>>demandNum;
	fin>>lineNum;
	fin>>busNum;
	fin>>horizon;
	fin.close();
	return 0;
}


//************read ��������***********************
int readNetData(char* netdata,
				IloEnv env,                                   
				IloNumArray& unitLocation,               //��������bus���
				IloNumArray& demandLocation,            //��������bus���
				Matrix2& demand,                      //����������ϵ��
				IloNumArray& lineCap                     //��������
				)
{
	ifstream fin(netdata,ios::in);
	if(!fin) env.out()<<"problem with file:"<<netdata <<endl;
	int i,t;

	for(i=0;i<thUnitNum;i++)
	{
		fin>> unitLocation[i];
	}

	for(i=0;i< demandNum;i++)
	{
		fin>> demandLocation[i];
	}
	/**
	for(i=0;i< demandNum;i++)
	{
	fin>> demand[i];
	}
	**/

	for(i=0;i<demandNum;i++)
	{
		for(t=1;t<cycle+1;t++)
		{
			fin>>demand[i][t];
		}
	}


	for(i=0;i<lineNum;i++)
	{
		fin>> lineCap [i];
	}
	return 0;
}

////***************��gama**********
int readGama(char* gamgadata,IloEnv env,Matrix2& gama)
{
	int i,j;
	ifstream fin(gamgadata,ios::in);		
	if(!fin) 
		env.out()<<"problem with "<< gamgadata<<endl;
	//read gama
	for(i=0;i<lineNum;i++)
	{
		for(j=0;j<busNum;j++)
		{
			fin>>gama[i][j];
		}
	}
	return 0;
}




//************��ȡ���������ݺ���************************
int readThUnitData(char* thunitdata,
				   IloEnv env,                                   //���廷������
				   IloNumArray& thminPower,                        //������С������
				   IloNumArray& thmaxPower,                        //������󷢵���
				   IloNumArray& thminDown,                         //������С�ػ�ʱ��
				   IloNumArray& thminUp,                           //������С����ʱ��
				   IloNumArray& thcoldUpTime,                      //������ʱ��
				   IloNumArray& thfuelCostPieceNum,                //ȼ�Ϸ������߶���
				   IloNumArray& thhotUpCost,                       //����������
				   IloNumArray& thcoldUpCost,			           //���������� IloNumArray& hotUpTime
				   IloNumArray& thdelta,						   //����
				   IloNumArray& thfirstLast,                       //��ĩ����Լ����ȡ0��1
				   IloNumArray& thmaxR,                            //���������	
				   IloNumArray& tha,							   //ȼ�Ϸ���������ϵ��a		
				   IloNumArray& thb,							   //ȼ�Ϸ���������ϵ��b		
				   IloNumArray& thc,						       //ȼ�Ϸ���������ϵ��c
				   IloNumArray& thminFuelCost,                        //������С�������
				   IloNumArray& thmaxFuelCost,                        //������󷢵����
				   IloNumArray& thinitState,                        //�����ʼ״̬
				   IloNumArray& thinitPower                        //�����ʼ������			 
				   )
{
	ifstream fin(thunitdata,ios::in);
	if(!fin) env.out()<<"problem with file:"<<thunitdata<<endl;
	int i;

	//read minPower
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thminPower[i];
	}

	//read maxPower
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thmaxPower[i];
	}

	//read minDown
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thminDown[i];
	}

	//read minUp
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thminUp[i];
	}

	//read coldUpTime
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thcoldUpTime[i];
	}

	//read thfuelCostPieceNum	
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thfuelCostPieceNum[i];
	}

	//read hotUpCost
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thhotUpCost[i];
	}

	//read coldUpCost
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thcoldUpCost[i];
	}

	//read delta
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thdelta[i];
	}

	//read firstlast
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thfirstLast[i];
	}

	//read maxR
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thmaxR[i];
	}

	//read a
	for(i=0;i<thUnitNum;i++)
	{
		fin>>tha[i];
	}

	//read b
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thb[i];
	}
	//read c
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thc[i];
	}

	for(i=0;i<thUnitNum;i++)
	{
		thminFuelCost[i]=tha[i]*thminPower[i]*thminPower[i]+thb[i]*thminPower[i]+thc[i];
		thmaxFuelCost[i]=tha[i]*thmaxPower[i]*thmaxPower[i]+thb[i]*thmaxPower[i]+thc[i];
	}
	//read thinitState
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thinitState[i];
	}
	//read thinitPower
	for(i=0;i<thUnitNum;i++)
	{
		fin>>thinitPower[i];
	}


	return 0;
}

//********the data of piece fuelcost********//
int pieceThUnitData(IloEnv env,
					IloNumArray& tha,							   //ȼ�Ϸ���������ϵ��a		
					IloNumArray& thb,							   //ȼ�Ϸ���������ϵ��b		
					IloNumArray& thc,							   //ȼ�Ϸ���������ϵ��b
					IloNumArray& thminPower,                        //������С������
					IloNumArray& thmaxPower,						//������󷢵���
					IloNumArray& thfuelCostPieceNum,                //�ֶ���
					Matrix2& thminPiecePower,                 //?????????
					Matrix2& thmaxPiecePower,
					Matrix2& thminFuelPieceCost,
					Matrix2& thmaxFuelPieceCost,
					Matrix2& thfuelCostPieceSlope
					) 
{
	int i,j,pieceNum;
	IloNum d;
	for(i=0;i<thUnitNum;i++)
	{
		pieceNum=thfuelCostPieceNum[i];
		d=(thmaxPower[i]-thminPower[i])/pieceNum;
		thminPiecePower[i][0]=thminPower[i];
		thminFuelPieceCost[i][0]=tha[i]*thminPiecePower[i][0]*thminPiecePower[i][0]+thb[i]*thminPiecePower[i][0]+thc[i];

		thmaxPiecePower[i][pieceNum-1]=thmaxPower[i];
		thmaxFuelPieceCost[i][pieceNum-1]=tha[i]*thmaxPiecePower[i][pieceNum-1]*thmaxPiecePower[i][pieceNum-1]+thb[i]*thmaxPiecePower[i][pieceNum-1]+thc[i];
		for(j=1;j<pieceNum;j++)
		{
			thminPiecePower[i][j]=thminPiecePower[i][j-1]+d;
			thmaxPiecePower[i][j-1]=thminPiecePower[i][j];
			thminFuelPieceCost[i][j]=tha[i]*thminPiecePower[i][j]*thminPiecePower[i][j]+thb[i]*thminPiecePower[i][j]+thc[i];
			thmaxFuelPieceCost[i][j-1]=thminFuelPieceCost[i][j];			
		}
		for(j=0;j<pieceNum;j++)
		{
			thfuelCostPieceSlope[i][j]=(thmaxFuelPieceCost[i][j]-thminFuelPieceCost[i][j])/d;
		}
	}
	return 0;
}

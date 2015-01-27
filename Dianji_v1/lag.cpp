/************************************************************************************
* The model shows a Lagrangian relaxation for a location-transportation problem.   *
* The original MIP is decomposed into two problems in order to deduce a multiplier *
* for a particular constraint based on Lagrange relaxation.						*
* Written by: Sanjay Ramanujan					  									*
* Version: 1.0, November 1st, 2002, Tested with CPLEX v8.0							*					
* For consistency, this is built using an equivalent AMPL model at:				*
* http://www.ampl.com/NEW/loop2.html												* 
************************************************************************************/

#include <ilcplex/ilocplex.h>

#define smallNumber 0.000001

ILOSTLBEGIN

	typedef IloArray<IloNumArray> TwoDMatrix;
typedef IloArray<IloNumVarArray> NumVarMatrix;

static
	void displayResults(IloModel, IloCplex&, IloBoolVarArray, NumVarMatrix, IloInt);

static
	double solveRelaxed(IloModel, IloBoolVarArray, NumVarMatrix, IloInt, IloNumArray);


int
	main2(int argc, char **argv) {
		IloEnv env;

		try {

			IloInt i=0, j=0, k=0;
			IloInt nbCities=0;
			IloInt build_limit=0;


			IloNumArray send(env), request(env);

			TwoDMatrix ship_cost(env);



			///////////////// DATA FILE READING ////////////////////////////////
			const char* filename  = "data/lagdata.dat";
			if (argc > 1)
				filename = argv[1];
			ifstream file(filename);
			if (!file) {
				cerr << "ERROR: could not open file '" << filename
					<< "' for reading" << endl;
				cerr << "usage:   " << argv[0] << " <file>" << endl;
				throw(-1);
			}

			file >> build_limit >> send >> request >> ship_cost ;
			nbCities = send.getSize();

			env.out() << "Total number of cities: " << nbCities << endl;
			env.out() <<"build_limit value: " << build_limit << endl;


			IloBool consistentData = (request.getSize() == nbCities && ship_cost.getSize() == nbCities);
			if (!consistentData) {
				cerr << "ERROR: data file '"
					<< filename << "' contains inconsistent data" << endl;
				throw(-1);
			}



			/////////////////// DECISION VARIABLES WITH NAMES  /////////////////////////////
			IloBoolVarArray  Build(env, nbCities);
			NumVarMatrix Ship(env, nbCities);
			IloNumArray mult(env, nbCities);

			for(k=0; k < nbCities; k++) {
				mult[k] = 0.0;
			}

			for(i=0; i <nbCities; i++) {
				Ship[i] = IloNumVarArray(env, nbCities, 0, CPX_INFBOUND, ILOINT);
			}

			for(i=0; i < nbCities; i++) {
				for(j=0; j < nbCities; j++) {
					Ship[i][j] = IloNumVar(env, 0, CPX_INFBOUND, ILOINT); }
			}


			IloModel model(env);

			IloCplex cplex(env);

			//		ofstream fout("mylog.log");
			//	    cplex.setOut(fout);


			cplex.setOut(env.getNullStream());

			cplex.setWarning(env.getNullStream());

			cplex.setError(env.getNullStream());


			char *buffer = NULL;
			buffer = new char[200];

			for(i=0; i< nbCities; i++) {
				sprintf(buffer, "Build(%d)",i);
				Build[i].setName(buffer);
			}

			for(i=0; i< nbCities; i++) {
				for(j=0; j< nbCities; j++) {
					sprintf(buffer, "Ship(%d,%d)",i,j);
					Ship[i][j].setName(buffer);
				}
			}

			delete[] buffer;

			IloInt  iter_limit;
			cout << "Enter number of iterations desired: " << endl;
			cin >> iter_limit;


			////////////DEVELOP GENERIC MODEL //////////////////////////

			//shipping objective function
			IloExpr shipobj(env);
			for(i=0; i< nbCities; i++) {
				shipobj += IloScalProd(Ship[i],ship_cost[i]);
			}

			IloObjective shipping_obj(env);
			shipping_obj.setName("shipping_obj");
			shipping_obj = IloAdd(model,IloMinimize(env, shipobj));


			// supply_constraint
			IloRangeArray supply_constr(env);
			for(i=0; i< nbCities; i++) {
				model.add(IloSum(Ship[i]) <= Build[i] * send[i]);
			}

			//model.add(supply_constr);

			// limit_constraint
			IloRange limit_constr(env, -IloInfinity, IloSum(Build), build_limit, "limit_constr");
			model.add(limit_constr);



			////////////////////// SOLVE THE RELAXED MODEL NOW //////////////////////////////////////

			double LB = solveRelaxed(model, Build, Ship, nbCities, request);

			env.out() << endl << "LP Relaxation value: " << LB << endl << endl;

			model.remove(shipping_obj);

			IloModel mlowerbound(env);

			mlowerbound.add(model);

			IloCplex clb(env);

			clb.extract(mlowerbound);

			clb.setOut(env.getNullStream());

			clb.setWarning(env.getNullStream());

			clb.setError(env.getNullStream());

			IloModel mupperbound(env);

			shipping_obj = IloAdd(mupperbound,IloMinimize(env, shipobj));

			for(j=0; j< nbCities; j++) {
				IloExpr expD(env);
				for(i=0; i< nbCities; i++) {
					expD += Ship[i][j];
				}
				mupperbound.add(expD >= request[j]);
				expD.end();
			}

			IloCplex cub(env);

			cub.setOut(env.getNullStream());

			cub.setWarning(env.getNullStream());

			cub.setError(env.getNullStream());


			cub.extract(mupperbound);



			///////////////// DEFINE LAGRANGE VARIABLES  ////////////////////////////////
			IloInt same		  = 0,
				same_limit = 3;


			IloNum scale = 1.0,
				norm  = 0.0,
				step  = 0.0,
				UB	  = 0.0;


			IloNumArray LBlog(env, iter_limit);
			for(k=0; k< iter_limit; k++) {
				LBlog[k] = 0.0;
			}

			LBlog[0] = LB;


			IloNumArray slack(env, nbCities);
			for (i=0; i< nbCities; i++) {
				slack[i] = 0.0;
			}


			IloNumArray temp(env, nbCities);
			for(i=0; i< nbCities; i++) {
				temp[i] = IloMax(ship_cost[i]);
				UB = IloSum(temp);
			}
			temp.end();


			IloNumArray UBlog(env, iter_limit);
			UBlog[0] = UB;


			IloNumArray scalelog(env, iter_limit);
			IloNumArray steplog(env, iter_limit);

			IloNum Lagrangian = 0.0;



			////////// BEGIN LAGRANGE ITERATIONS HERE /////////////////////////////////////

			for(k=0; k<iter_limit; k++) {

				env.out() << "  " << endl;

				env.out() << "  ITERATION  " << k+1 << endl;

				IloExpr lagrobj1(env), lagrobj2(env), lagrobj3(env), lagrobj(env);
				for(i=0; i< nbCities; i++) {
					lagrobj1 += IloScalProd(Ship[i],ship_cost[i]);
				}
				for(j=0; j< nbCities; j++) {
					lagrobj2 += mult[j] * request[j];
				}
				for(j=0; j< nbCities; j++) {
					lagrobj3 += IloScalProd(mult, Ship[j]);
				}


				lagrobj += lagrobj1 + lagrobj2 - lagrobj3;

				IloObjective lagrange_obj(env);

				lagrange_obj.setName("lagrange_obj");

				lagrange_obj = IloAdd(mlowerbound,IloMinimize(env, lagrobj));

				lagrobj.end();


				if (clb.solve()) {

					Lagrangian = clb.getObjValue();

					env.out() << "lower bound model obj value " << Lagrangian << endl;

					TwoDMatrix tempShipMatrix(env, nbCities);
					for(i=0; i< nbCities; i++) {
						tempShipMatrix[i] = IloNumArray(env, nbCities);
					}

					for(i=0; i< nbCities; i++) {
						clb.getValues(tempShipMatrix[i], Ship[i]);
					}


					IloNumArray tempSumArray(env, nbCities);

					for(i=0; i< nbCities; i++) {
						for(j=0; j< nbCities; j++) {
							tempSumArray[i] += tempShipMatrix[j][i]; }
					}


					for(i=0; i< nbCities; i++) {
						slack[i] = tempSumArray[i] - request[i];
					}

					tempShipMatrix.end();
					tempSumArray.end();


					if (Lagrangian > LB + smallNumber) {
						LB = clb.getObjValue();
						same = 0;
					}
					else {	same = same + 1;  }
				}


				if (same == same_limit) {
					scale = scale/2;
					same  = 0;
				}


				IloNumArray normtemp(env, nbCities);

				for(j=0; j< nbCities; j++) {
					normtemp[j] = IloPower(slack[j], 2); }

				norm = IloSum(normtemp);
				normtemp.end();


				step = scale * ( (UB - Lagrangian) / norm );

				norm = 0.0;


				IloNumArray SBuild(env, nbCities);

				IloNum tolerance = clb.getParam(IloCplex::EpInt);

				for(j=0; j< nbCities; j++) {

					if(clb.getValue(Build[j]) > 1 - tolerance) {

						SBuild[j] = clb.getValue(Build[j]);
					}
				}

				IloRangeArray ub_supply_constr(env);

				for(i=0; i< nbCities; i++) {
					ub_supply_constr.add(IloSum(Ship[i]) <= SBuild[i] * send[i]);
				}

				mupperbound.add(ub_supply_constr);

				if ( (IloScalProd(send, SBuild) ) >=
					( IloSum(request) - 1.0/IloPower(10,8) ) ) {

						if (cub.solve()) {

							cub.solve();

							env.out() << "upper bound model obj value " << cub.getObjValue() << endl;

							if(cub.getObjValue() <= UB) {
								UB = cub.getObjValue();
							}
							else {	UB = UB; }
						}
				}

				LBlog[k] 	=  LB;
				UBlog[k]	=  UB;
				scalelog[k]	=  scale;
				steplog[k]  =  step;

				for(j=0; j< nbCities; j++) {

					if(mult[j] - (step * slack[j]) > 0) {
						mult[j] = mult[j] - (step  * slack[j]);
					}

					else {	mult[j] = 0; }
				}

				//remove for next set of runs
				mupperbound.remove(ub_supply_constr);
				ub_supply_constr.end();
				mlowerbound.remove(lagrange_obj);
				lagrange_obj.end();
				if (k == (iter_limit-1)) {

					cout << " " << endl << endl;

					env.out() << " Results " << endl << endl;

					for(i=0; i< iter_limit; i++) {
						cout << "LBlog[" << i << "]" << LBlog[i] << endl; }

					cout << " " << endl;

					for(i=0; i< iter_limit; i++) {
						cout << "UBlog[" << i << "]" << UBlog[i] << endl; }

					cout << " " << endl;

					for(i=0; i< iter_limit; i++)  {
						cout << "scalelog[" << i << "]" << scalelog[i] << endl; }

					cout << " " << endl;

					for(i=0; i< iter_limit; i++) {
						cout << "steplog[" << i << "]" << steplog[i] << endl; }

					cout << " " << endl;


				}


				env.out() << " " << endl;


			}	// end of for(k=0; k< iter_limit; k++)


		}
		catch(IloException &e) {
			env.out() << "ERROR: " << e << endl;
		}
		catch(...){
			env.out() << "Unknown exception" << endl;
		}

		env.end();

		return 0;
}


static
	void displayResults(IloModel mdl,
	IloCplex& cplex,
	IloBoolVarArray Build,
	NumVarMatrix Ship,
	IloInt nbCities)
{
	IloEnv env = mdl.getEnv();
	IloInt j=0, k=0;
	env.out() << "Optimal value: " << cplex.getObjValue() << endl << endl;

	env.out() << " --------------------------- " << endl;

	IloNum tolerance = cplex.getParam(IloCplex::EpInt);

	for(k=0; k< nbCities; k++) {
		if(cplex.getValue(Build[k]) > 1 - tolerance)
			env.out() << "Build["<< k <<"] " <<" = " << cplex.getValue(Build[k]) << endl;
	}

	env.out() << " --------------------------- " << endl;

	for(k=0; k< nbCities; k++)
		for(int j=0; j< nbCities; j++) 		{
			if(cplex.getValue(Ship[k][j]) >= 1 - tolerance)
				env.out() << "Ship["<< k << "]" << "[" << j <<"] " <<" = " << cplex.getValue(Ship[k][j]) << endl;
		}

		env.out() << "Time: " << cplex.getTime() << endl << endl;

		env.end();
}


static
	double solveRelaxed(IloModel mdl,
	IloBoolVarArray bvar,
	NumVarMatrix nvar,
	IloInt nbCities,
	IloNumArray req)

{
	IloEnv env = mdl.getEnv();
	IloModel relax(env);
	relax.add(mdl);

	relax.add(IloConversion(env, bvar, ILOFLOAT));

	for(int i=0; i<nbCities; i++) {
		relax.add(IloConversion(env, nvar[i], ILOFLOAT)); }

	for(int j=0; j< nbCities; j++) {
		IloExpr expR(env);
		for(int i=0; i< nbCities; i++) {
			expR += nvar[i][j];
		}
		relax.add( expR >= req[j]);
		expR.end();
	}

	IloCplex cplex(env);

	cplex.setOut(env.getNullStream());

	cplex.extract(relax);

	cplex.solve();
	return cplex.getObjValue();

	env.end();
}


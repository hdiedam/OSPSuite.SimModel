#ifdef WIN32_PRODUCTION
#pragma managed(push,off)
#endif

#ifdef WIN32
#pragma warning(disable:4786)
#endif

#include "SimModel/Species.h"
#include "SimModel/Simulation.h"
#include <vector>
#include "XMLWrapper/XMLHelper.h"
#include "SimModel/SimulationTask.h"
#include "SimModel/ParameterSensitivity.h"
#include <map>

#ifdef WIN32_PRODUCTION
#pragma managed(pop)
#endif

namespace SimModelNative
{

using namespace std;

Species::Species(void)
{
	m_ODEScaleFactor = 1.0;
	_DEScaleFactorInv = 1.0;
	m_ODEIndex = DE_INVALID_INDEX;
	_simulationStartTime = 0.0;
	_rhsFormulaListSize = 0;
	_RHS_noOfUsedVariables = 0;
	_RHS_UsedVariablesIndices = NULL;
	
	_negativeValuesAllowed = true;
}

Species::~Species(void)
{
	_rhsFormulaList.FreeVector();
	if (_RHS_UsedVariablesIndices != NULL)
	{
		delete[] _RHS_UsedVariablesIndices;
		_RHS_UsedVariablesIndices = NULL;
	}
//	_parameterSensitivities.clear();
}

double Species::GetODEScaleFactor () const
{
    return m_ODEScaleFactor;
}

void Species::SetODEScaleFactor (double p_ODEScaleFactor)
{
	if (p_ODEScaleFactor <= 0.0)
		throw ErrorData(ErrorData::ED_ERROR, "Species::SetODEScaleFactor", "Species id="+_idAsString+": Scale factor must be > 0");

    m_ODEScaleFactor=p_ODEScaleFactor;
	_DEScaleFactorInv = 1.0 / m_ODEScaleFactor;
}

void Species::LoadFromXMLNode (const XMLNode & pNode)
{
	// ---- XML sample
	//<Species Id="S1" Name="Drug" Path="Liver/Cells" InitialValueFormulaId="F2" Unit="�mol/l">
	//	<ScaleFactor>17.2</ScaleFactor>
	//	<RHSFormulaList>
	//		<RHSFormula Id="F3"/>
	//		<RHSFormula Id="F4"/>
	//	</RHSFormulaList>
	//</Species>

	//---- common quantity part
	Quantity::LoadFromXMLNode(pNode);

	//---- species specific part
	SetODEScaleFactor(pNode.GetChildNodeValue(XMLConstants::ScaleFactor, 1.0));

	_negativeValuesAllowed = (pNode.GetAttribute(XMLConstants::NegativeValuesAllowed, _negativeValuesAllowed ? 1 : 0) == 1);

	if ((_containerPath.find("SalivaGland") != string::npos) ||
		(_containerPath.find("IgG_Source") != string::npos) ||
		(_containerPath.find("ParticleBin_") != string::npos)||
		(_containerPath.find("|Applications|") != string::npos))
		_negativeValuesAllowed = true;
}

string Species::getFormulaXMLAttributeName()
{
	return XMLConstants::InitialValueFormulaId;
}

void Species::XMLFinalizeInstance (const XMLNode & pNode, Simulation * sim)
{
	const char * ERROR_SOURCE = "Species::XMLFinalizeInstance";

	//---- common quantity part
	Quantity::XMLFinalizeInstance(pNode, sim);

	//---- species specific part

	XMLNode rhsFormulasNode = pNode.GetChildNode(XMLConstants::RHSFormulaList);

	if(!rhsFormulasNode.IsNull())
	{
		for (XMLNode pChild = rhsFormulasNode.GetFirstChild(); !pChild.IsNull();pChild = pChild.GetNextSibling()) 
		{
			assert(pChild.HasName(XMLConstants::RHSFormula));

			long formulaId = (long)pChild.GetAttribute(XMLConstants::Id, INVALID_QUANTITY_ID);

			Formula * rhsFormula = sim->Formulas().GetObjectById(formulaId);

			if (rhsFormula == NULL)
				throw ErrorData(ErrorData::ED_ERROR, ERROR_SOURCE, "Formula with id="+XMLHelper::ToString(formulaId)+" not found (in species with id=" + _idAsString+")");

			_rhsFormulaList.Add(rhsFormula);
		}
	}

	_simulationStartTime = sim->GetStartTime();

	_rhsFormulaListSize = _rhsFormulaList.size(); //cache for performance optimization
}

bool Species::IsConstantDuringCalculation()
{
	return ((_rhsFormulaListSize == 0) && !_isChangedBySwitch);
}

bool Species::IsConstant(bool forCurrentRunOnly)
{
	return (Quantity::IsConstant(forCurrentRunOnly) && IsConstantDuringCalculation());
}

double Species::GetValue (const double * y, double time, ScaleFactorUsageMode scaleFactorMode)
{
//	assert(_valueFormula==NULL);

	if (!IsConstantDuringCalculation())
	{
		assert(time == _simulationStartTime);
	}

	return GetInitialValue(y, _simulationStartTime);
//	return _value; 
}

double Species::GetInitialValue (const double * y, double time)
{
	//getting initial value must IGNORE scale factor
	if (_valueFormula)
		return _valueFormula->DE_Compute(y, time, IGNORE_SCALEFACTOR);

	return
		_value;
}

vector < HierarchicalFormulaObject * > Species::GetUsedHierarchicalFormulaObjects ()
{
	if (_valueFormula == NULL)
		return std::vector < HierarchicalFormulaObject * > ();

	return _valueFormula->GetUsedHierarchicalFormulaObjects();
}

bool Species::SimplifyRHSList()
{
	int i;
	bool simplified = false;

	if (_rhsFormulaList.size() == 0)
		return false; //no more rhs formulas available

	//TODO implement TObjectList.Remove()
	vector<Formula *> nonZeroFormulas;

	for (i=0; i<_rhsFormulaList.size(); i++)
	{
		Formula * rhsFormula = _rhsFormulaList[i];
		
		if (rhsFormula->IsZero())
			simplified = true;
		else
			nonZeroFormulas.push_back(rhsFormula);
	}

	_rhsFormulaList.FreeVector();
	for (unsigned j=0; j<nonZeroFormulas.size(); j++)
		_rhsFormulaList.Add(nonZeroFormulas[j]);

	_rhsFormulaListSize = _rhsFormulaList.size(); //cache for performance optimization

	return simplified;
}

void Species::DE_Jacobian (double * * jacobian, const double * y, const double time, const int iEquation, const double preFactor)
{
	// if called, species MUST be constant for current RUN
	// In this case: nothing to do
	assert(IsConstant(true));
}

int Species::GetODEIndex () const
{
    return m_ODEIndex;
}

void Species::DE_SetSpeciesIndex (int & iEquationNumber)
{
	m_ODEIndex = iEquationNumber++;
}

void Species::DE_Rhs (double * ydot, const double * y, const double time)
{
	for (int i=0; i<_rhsFormulaListSize; i++) 
		ydot[m_ODEIndex] += _rhsFormulaList[i]->DE_Compute(y, time, USE_SCALEFACTOR);

	ydot[m_ODEIndex] *= _DEScaleFactorInv; 
}

void Species::DE_Jacobian (double * * jacobian, const double * y, const double time)
{
	for (int i=0; i<_rhsFormulaListSize; i++) 
		_rhsFormulaList[i]->DE_Jacobian(jacobian, y, time, m_ODEIndex, _DEScaleFactorInv);
}


bool Species::RHSDependsOn(int DE_VariableIndex)
{
	if (_RHS_noOfUsedVariables == 0)
		return false;

	if ((DE_VariableIndex < _RHS_UsedVariablesIndices[0]) || 
		(DE_VariableIndex > _RHS_UsedVariablesIndices[_RHS_noOfUsedVariables-1]))
		return false; //used indices are sorted!

	for(int i=0; i<_RHS_noOfUsedVariables; i++)
	{
		if (_RHS_UsedVariablesIndices[i] == DE_VariableIndex)
			return true;
	}

	return false;
}

void Species::FillWithInitialValue(const double * speciesInitialValuesScaled)
{
	double initialValue = GetInitialValue(speciesInitialValuesScaled, _simulationStartTime); //GetValue(NULL, 0.0, IGNORE_SCALEFACTOR);

	//---- set value as unscaled for constant species!!!
	initialValue *= m_ODEScaleFactor;

	//---- set initial formula to NULL and set initial value instead
	SetConstantValue(initialValue);

	//---- redim values vector with size 1 and set the first (only) value to initial value
	SetTheOnlyValue(initialValue);
}

void Species::RescaleValues ()
{
	if (m_ODEScaleFactor ==1.0) 
		return; //nothing to do for scale factor=1
	
	for(int i =0; i<_valuesSize;i++)
		_values[i] *= m_ODEScaleFactor;
}

void Species::SetValuesBelowAbsTolLevelToZero(double absTol)
{
	SimulationTask::SetValuesBelowAbsTolLevelToZero(_values, _valuesSize, absTol);
}

bool Species::Simplify (bool forCurrentRunOnly)
{
	return (Quantity::Simplify(forCurrentRunOnly) && IsConstantDuringCalculation());
}

void Species::WriteMatlabCode (std::ostream & mrOut)
{
	//SimModel indexing starts at 0, Matlab indexing at 1
	mrOut<<"dy("<<m_ODEIndex+1<<") = ";

	for (int i=0; i<_rhsFormulaListSize; i++)
	{
		mrOut<<" ..."<<std::endl<<"            ";
	
		if (i!=0)
			mrOut<<"+";

		_rhsFormulaList[i]->WriteMatlabCode(mrOut);
	}
	mrOut<<";"<<endl;
}

//will be called between Load and Finalize of the parent simulation
//must set all properties of the quantity info (name, unit, id, ...)
//The quantity formula may not be evaluated at this time point
//The only exception are constant formulas, which were already replaced
//by values in Simulation::LoadFromXXX
void Species::InitialFillInfo(SpeciesInfo & info)
{
	//---- fill common quantity properties first
	Quantity::InitialFillInfo(info);

	//---- set species specific properties
	info.SetScaleFactor(m_ODEScaleFactor);
}

//will be called after the finalize of the parent simulation
//only formula/value info must be updated
//Species initial values and sim. start time are required for the formula evaluation
void Species::FillInfo(SpeciesInfo & info, 
					   const double * speciesInitialValues,
					   double simulationStartTime)
{
	//---- fill common quantity properties first
	Quantity::FillInfo(info, speciesInitialValues, simulationStartTime);

	//---- set species specific properties
	info.SetScaleFactor(m_ODEScaleFactor);
}

void Species::CacheRHSUsedVariables(const std::set<int> & DEVariblesUsedInSwitchAssignments)
{
	//---- reset used variables
	_RHS_noOfUsedVariables = 0;

	if (_RHS_UsedVariablesIndices != NULL)
	{
		delete[] _RHS_UsedVariablesIndices;
		_RHS_UsedVariablesIndices = NULL;
	}

	//---- insert used variables from all rhs formulas
	set<int>::const_iterator iter;
	int i;
	set<int> usedVariables;

	for (i=0; i<_rhsFormulaListSize; i++)
	{
		_rhsFormulaList[i]->AppendUsedVariables(usedVariables, DEVariblesUsedInSwitchAssignments);
	}

	//---- now cache used variables (indices)
	if (usedVariables.size()==0)
		return; //RHS does not use any variable

	_RHS_noOfUsedVariables = usedVariables.size();

	_RHS_UsedVariablesIndices = new int [_RHS_noOfUsedVariables];

	for(iter = usedVariables.begin(), i=0; iter != usedVariables.end(); iter++, i++)
	{
		_RHS_UsedVariablesIndices[i] = *iter;
	}
}

vector<bool> Species::RHSDependencyVector(int numberOfVariables)
{
	vector<bool> dependencyInfo;

	int i;

	for(i=0; i<numberOfVariables; i++)
		dependencyInfo.push_back(false);

	for(i=0; i<_RHS_noOfUsedVariables; i++)
		dependencyInfo[_RHS_UsedVariablesIndices[i]] = true;

	return dependencyInfo;
}

void Species::SetODEIndex(int newIndex)
{
	m_ODEIndex = newIndex;
}

//indexMap contains the mapping (<Old DE index>, <New DE index>)
void Species::ChangeIndicesOfRHSUsedVariables(map<unsigned int, unsigned int> & indexMap)
{
	const char * ERROR_SOURCE = "Species::ChangeIndicesOfRHSUsedVariables";
	int i;
	
	//---- first push all NEW indices into a set (in order to get them sorted at the end)
	set<int> newIndices;
	for(i=0; i<_RHS_noOfUsedVariables; i++)
		newIndices.insert(indexMap[_RHS_UsedVariablesIndices[i]]);
	
	//make sure the number of new indices matches the number of old indices
	if(newIndices.size() != _RHS_noOfUsedVariables)
		throw ErrorData(ErrorData::ED_ERROR, ERROR_SOURCE, "Number of new indices does not match number of old indices");

	//---- now replace old indices (sorted) with new indices (sorted)
	set<int>::const_iterator iter;
	for(iter = newIndices.begin(), i=0; iter != newIndices.end(); iter++, i++)
	{
		_RHS_UsedVariablesIndices[i] = *iter;
	}
}

//The half-bandwidths are set such that the nonzero locations (i, j) 
//in the banded Jacobian satisfy 
// -lowerHalfBandWidth <= j-i <= upperHalfBandWidth
//
// (where i is the ODE index of the current variable)
void Species::GetRHSUsedBandRange(int & upperHalfBandWidth, int & lowerHalfBandWidth)
{
	upperHalfBandWidth = 0;
	lowerHalfBandWidth = 0;

	for(int i=0; i<_RHS_noOfUsedVariables; i++)
	{
		int diff = _RHS_UsedVariablesIndices[i] - m_ODEIndex;

		upperHalfBandWidth = max(upperHalfBandWidth, diff);
		lowerHalfBandWidth = max(lowerHalfBandWidth, -diff);
	}
}

bool Species::NegativeValuesAllowed(void)
{
	return _negativeValuesAllowed;
}

}//.. end "namespace SimModelNative"
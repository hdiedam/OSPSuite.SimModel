#ifdef WIN32_PRODUCTION
#pragma managed(push,off)
#endif

#ifdef WIN32
#pragma warning(disable:4786)
#endif

#include "SimModel/QuantityReference.h"
#include "SimModel/Species.h"
#include "XMLWrapper/XMLHelper.h"
#include "SimModel/Simulation.h"
#include "SimModel/Parameter.h"

#ifdef WIN32_PRODUCTION
#pragma managed(pop)
#endif

namespace SimModelNative
{

using namespace std;

QuantityReference::QuantityReference(void)
{
	_quantity=NULL;
	_isTime = false;
	_isParameter= false;
	_isObserver= false;
	_isSpecies= false;
	_isReference = false;
	_quantityId = INVALID_QUANTITY_ID;
}

void QuantityReference::SetupFrom(Parameter * parameter, const std::string & alias)
{
	_quantity=parameter;
	_isTime = false;
	_isParameter= true;
	_isObserver= false;
	_isSpecies= false;
	_isReference=false;

	_quantityId = parameter->GetId();
	_alias = alias;
}

QuantityReference::~QuantityReference(void)
{
}

void QuantityReference::SetParentFormulaInfo(const string & parentFormulaInfo)
{
	_parentFormulaInfo = parentFormulaInfo;
}

bool QuantityReference::IsTime()
{
	return _isTime;
}

bool QuantityReference::IsParameter()
{
	return _isParameter;
}

bool QuantityReference::IsObserver()
{
	return _isObserver;
}

bool QuantityReference::IsSpecies()
{
	return _isSpecies;
}

bool QuantityReference::SimplifyQuantity(bool forCurrentRunOnly)
{
	if (IsTime())
		return false;

	return _quantity->Simplify(forCurrentRunOnly);
}

string QuantityReference::GetAlias(void) const
{
	return _alias;
}

void QuantityReference::SetAlias(string alias)
{
	_alias = alias;
}

double QuantityReference::GetODEScaleFactor () const
{
    Species * species = GetSpecies();
    assert(species!=NULL);
    
    return species->GetODEScaleFactor ();
}

void QuantityReference::LoadFromXMLNode (const XMLNode & pNode)
{
	_quantityId = (long)pNode.GetAttribute(XMLConstants::Id,INVALID_QUANTITY_ID);
	_alias      = pNode.GetAttribute(XMLConstants::Alias);
		
	//Set Type of object reference
	if (pNode.HasName(XMLConstants::Parameter))
	{
		if (_quantityId == TIME_QUANTITY_ID) 
			_isTime = true;
		else
			_isParameter = true;
	}
	else if (pNode.HasName(XMLConstants::Observer))
		_isObserver = true;
	else if (pNode.HasName(XMLConstants::Variable))
		_isSpecies = true;
	else if (pNode.HasName(XMLConstants::Reference))
	{
		if (_quantityId == TIME_QUANTITY_ID)
			_isTime = true;
		else 
			_isReference = true; //mark quantity ref as general reference, quantity type will be determined later
	}
	else
		throw ErrorData(ErrorData::ED_ERROR, "QuantityReference::LoadFromXMLNode", "Unknown object reference");

}

void QuantityReference::XMLFinalizeInstance (const XMLNode & pNode, Simulation * sim)
{
	const char * ERROR_SOURCE = "QuantityReference::XMLFinalizeInstance";

	if (!IsTime()) // "Time" is not a real quantity
	{
		_quantity = sim->AllQuantities().GetObjectById(_quantityId);

		if (_quantity == NULL)
			throw ErrorData(ErrorData::ED_ERROR, ERROR_SOURCE, _parentFormulaInfo+": object with id="+XMLHelper::ToString(_quantityId)+" not found");

		if (_isReference) //quantity comes from the general <ReferenceList>
		{
			if (dynamic_cast<Species *>(_quantity) != NULL)
				_isSpecies = true;
			else if (dynamic_cast<Parameter *>(_quantity) != NULL)
				_isParameter = true;
			else if (dynamic_cast<Observer *>(_quantity) != NULL)
				_isObserver = true;
			else
				throw ErrorData(ErrorData::ED_ERROR, ERROR_SOURCE, _parentFormulaInfo+": object with id="+XMLHelper::ToString(_quantityId)+" has invalid type");

			_isReference = false;
		}
		else
		{
			if (_isSpecies && (dynamic_cast<Species *>(_quantity) == NULL))
				throw ErrorData(ErrorData::ED_ERROR, ERROR_SOURCE, _parentFormulaInfo+": object with id="+XMLHelper::ToString(_quantityId)+" should be a species");

			if (_isParameter && (dynamic_cast<Parameter *>(_quantity) == NULL))
				throw ErrorData(ErrorData::ED_ERROR, ERROR_SOURCE, _parentFormulaInfo+": object with id="+XMLHelper::ToString(_quantityId)+" should be a parameter");

			if (_isObserver && (dynamic_cast<Observer *>(_quantity) == NULL))
				throw ErrorData(ErrorData::ED_ERROR, ERROR_SOURCE, _parentFormulaInfo+": object with id="+XMLHelper::ToString(_quantityId)+" should be a observer");
		}
	}
}

HierarchicalFormulaObject * QuantityReference::GetHierarchicalFormulaObject(void)
{
	if (_quantity == NULL)
		return NULL; //e.g. "Time"

	return _quantity->GetHierarchicalFormulaObject();
}

double QuantityReference::GetValue (const double * y, double time, ScaleFactorUsageMode scaleFactorMode)
{
	if (_isTime)
		return time;
	
	assert(_quantity!=NULL);
	
	return _quantity->GetValue(y, time, scaleFactorMode);
}

void QuantityReference::DE_Jacobian (double * * jacobian, const double * y, const double time, const int iEquation, const double preFactor)
{
	if (preFactor == 0.0)
		return;

	if (_isTime)
		return; //nothing to do

	_quantity->DE_Jacobian(jacobian, y, time, iEquation, preFactor);
}

int QuantityReference::GetODEIndex () const
{
    Species * species = GetSpecies();
    assert(species!=NULL);
    
    return species->GetODEIndex ();
}

Species * QuantityReference::GetSpecies() const
{
	return dynamic_cast<Species *> (_quantity);
}

bool QuantityReference::IsConstant(bool forCurrentRunOnly)
{
	if (_isTime)
		return false;

	return _quantity->IsConstant(forCurrentRunOnly);
}

bool QuantityReference::IsChangedBySwitch(void)
{
	if (_isTime)
		return false;

	return _quantity->IsChangedBySwitch();
}

void QuantityReference::AppendUsedVariables(set<int> & usedVariblesIndices, const set<int> & variblesIndicesUsedInSwitchAssignments)
{
	if (_quantity == NULL)
		return;

	if(!_isParameter)
		return;

	_quantity->AppendUsedVariables(usedVariblesIndices,variblesIndicesUsedInSwitchAssignments);
}

void QuantityReference::UpdateIndicesOfReferencedVariables()
{
	if (_quantity == NULL)
		return;

	if(!_isParameter)
		return;

	_quantity->UpdateIndicesOfReferencedVariables();
}

}//.. end "namespace SimModelNative"
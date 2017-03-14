#ifdef WIN32_PRODUCTION
#pragma managed(push,off)
#endif

#include "SimModel/UnaryFunctionFormula.h"
#include "SimModel/FormulaFactory.h"
#include "SimModel/GlobalConstants.h"
#include <assert.h>
#include <algorithm>

#ifdef WIN32_PRODUCTION
#pragma managed(pop)
#endif

namespace SimModelNative
{

using namespace std;

//-------------------------------------------------------------------
//---- Unary function formula (base for all unary function formulas)
//-------------------------------------------------------------------

UnaryFunctionFormula::UnaryFunctionFormula (string funcName)
{
	m_FunctionName = funcName;
	m_ArgumentFormula = NULL;
}

UnaryFunctionFormula::~UnaryFunctionFormula ()
{
	if (m_ArgumentFormula!=NULL)
	{
		delete m_ArgumentFormula;
		m_ArgumentFormula = NULL;
	}
}

bool UnaryFunctionFormula::IsZero(void)
{
	//TODO
	return false;
}

void UnaryFunctionFormula::LoadFromXMLNode (const XMLNode & pNode)
{
	// Check if the current tag is actually the one we expect
	assert(pNode.HasName(m_FunctionName));

	m_ArgumentFormula = FormulaFactory::CreateFormula(pNode.GetFirstChild().GetNodeName());
	m_ArgumentFormula->LoadFromXMLNode(pNode.GetFirstChild());
}

void UnaryFunctionFormula::XMLFinalizeInstance (const XMLNode & pNode, Simulation * sim)
{
	// Check if the current tag is actually the one we expect
	assert(pNode.HasName(m_FunctionName));
		
	m_ArgumentFormula->XMLFinalizeInstance(pNode.GetFirstChild(), sim);
}


void UnaryFunctionFormula::SetQuantityReference (const QuantityReference & quantityReference)
{
	m_ArgumentFormula->SetQuantityReference(quantityReference);
}

double UnaryFunctionFormula::DE_Compute (const double * y, const double time, ScaleFactorUsageMode scaleFactorMode)
{
	double arg =  m_ArgumentFormula->DE_Compute(y, time, scaleFactorMode);

	return EvalFunction(arg);
}


void UnaryFunctionFormula::DE_Jacobian (double * * jacobian, const double * y, const double time, const int iEquation, const double preFactor)
{
	if (preFactor == 0.0)
		return;

	// jacobian    - Jacobian of the RHS of the ODE system
	// y           - solution at the current time
	// time      - current time
	// iEquation   - index of the equation number we're looking at
	// preFactor - multiplicative factor (to take product rule into account)

	//Formula:    (R)  = F(Arg)
	//Derivative: (R)' = F'(Arg) * (Arg)' 
	//F'(Arg) is the "jacobian multiplier", returned by "GetJacobianMultiplier"-routine of concrete unary function Formula
	
	double arg =  m_ArgumentFormula->DE_Compute(y, time, USE_SCALEFACTOR);
	double jacobianMultiplier = GetJacobianMultiplier(arg);
	
	if (jacobianMultiplier == 0.0)
		return;

	m_ArgumentFormula->DE_Jacobian(jacobian, y, time, iEquation, preFactor * jacobianMultiplier);

}

void UnaryFunctionFormula::Finalize()
{
	assert(m_ArgumentFormula != NULL);
	m_ArgumentFormula->Finalize();
}

void UnaryFunctionFormula::WriteFormulaMatlabCode (ostream & mrOut)
{
	string funcName = m_FunctionName;
	transform(funcName.begin(), funcName.end(), funcName.begin(), ::tolower);
	mrOut<<funcName.c_str()<<"(";
	m_ArgumentFormula->WriteMatlabCode(mrOut);
	mrOut<<")";
}

void UnaryFunctionFormula::AppendUsedVariables(set<int> & usedVariblesIndices, const set<int> & variblesIndicesUsedInSwitchAssignments)
{
	m_ArgumentFormula->AppendUsedVariables(usedVariblesIndices,variblesIndicesUsedInSwitchAssignments);
}

void UnaryFunctionFormula::UpdateIndicesOfReferencedVariables()
{
	m_ArgumentFormula->UpdateIndicesOfReferencedVariables();
}


//-------------------------------------------------------------------
//---- arccos
//-------------------------------------------------------------------

AcosFormula::AcosFormula () : UnaryFunctionFormula(FormulaName::Acos)
{

}

double AcosFormula::EvalFunction (double arg)
{
	return acos(arg);
}

double AcosFormula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = acos(arg)
	//Jacobian multiplier: F'(arg) = -1 / sqrt(1-arg^2)
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	return -1.0 / sqrt(1.0 - arg * arg);
}

//-------------------------------------------------------------------
//---- arcsin
//-------------------------------------------------------------------

AsinFormula::AsinFormula () : UnaryFunctionFormula(FormulaName::Asin)
{

}

double AsinFormula::EvalFunction (double arg)
{
	return asin(arg);
}

double AsinFormula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = asin(arg)
	//Jacobian multiplier: F'(arg) = 1 / sqrt(1-arg^2)
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	return 1.0 / sqrt(1.0 - arg * arg);
}

//-------------------------------------------------------------------
//---- arctan
//-------------------------------------------------------------------

AtanFormula::AtanFormula () : UnaryFunctionFormula(FormulaName::Atan)
{

}

double AtanFormula::EvalFunction (double arg)
{
	return atan(arg);
}

double AtanFormula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = atan(arg)
	//Jacobian multiplier: F'(arg) = 1 / (1+arg^2)
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	return 1.0 / (1.0 + arg * arg);
}

//-------------------------------------------------------------------
//---- cosh
//-------------------------------------------------------------------
CoshFormula::CoshFormula () : UnaryFunctionFormula(FormulaName::Cosh)
{

}

double CoshFormula::EvalFunction (double arg)
{
	return cosh(arg);
}

double CoshFormula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = cosh(arg)
	//Jacobian multiplier: F'(arg) = sinh(arg)
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	return sinh(arg);
}

//-------------------------------------------------------------------
//---- cos
//-------------------------------------------------------------------

CosFormula::CosFormula () : UnaryFunctionFormula(FormulaName::Cos)
{

}

double CosFormula::EvalFunction (double arg)
{
	return cos(arg);
}

double CosFormula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = cos(arg)
	//Jacobian multiplier: F'(arg) = -sin(arg)
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	return -sin(arg);
}

//-------------------------------------------------------------------
//---- exp
//-------------------------------------------------------------------

ExpFormula::ExpFormula () : UnaryFunctionFormula(FormulaName::Exp)
{

}

double ExpFormula::EvalFunction (double arg)
{
	return exp(arg);
}

double ExpFormula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = exp(arg)
	//Jacobian multiplier: F'(arg) = exp(arg)
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	return exp(arg);
}

//-------------------------------------------------------------------
//---- ln
//-------------------------------------------------------------------

LnFormula::LnFormula (std::string & funcName) : UnaryFunctionFormula(funcName)
{

}

double LnFormula::EvalFunction (double arg)
{
	return log(arg);
}

double LnFormula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = ln(arg)
	//Jacobian multiplier: F'(arg) = 1 / arg
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	return 1.0 / arg;
}

//-------------------------------------------------------------------
//---- log10
//-------------------------------------------------------------------

Log10Formula::Log10Formula () : UnaryFunctionFormula(FormulaName::Log10)
{

}

double Log10Formula::EvalFunction (double arg)
{
	return log10(arg);
}

double Log10Formula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = log10(arg)
	//Jacobian multiplier: F'(arg) = 1 / (arg * ln(10))
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	return 1.0 / (arg * log(10.0));
}

//-------------------------------------------------------------------
//---- sinh
//-------------------------------------------------------------------

SinhFormula::SinhFormula () : UnaryFunctionFormula(FormulaName::Sinh)
{

}

double SinhFormula::EvalFunction (double arg)
{
	return sinh(arg);
}

double SinhFormula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = sinh(arg)
	//Jacobian multiplier: F'(arg) = cosh(arg)
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	return cosh(arg);
}

//-------------------------------------------------------------------
//---- sin
//-------------------------------------------------------------------

SinFormula::SinFormula () : UnaryFunctionFormula(FormulaName::Sin)
{

}

double SinFormula::EvalFunction (double arg)
{
	return sin(arg);
}

double SinFormula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = sin(arg)
	//Jacobian multiplier: F'(arg) = cos(arg)
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	return cos(arg);
}

//-------------------------------------------------------------------
//---- sqrt
//-------------------------------------------------------------------

SqrtFormula::SqrtFormula () : UnaryFunctionFormula(FormulaName::Sqrt)
{

}

double SqrtFormula::EvalFunction (double arg)
{
	return sqrt(arg);
}

double SqrtFormula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = sqrt(arg)
	//Jacobian multiplier: F'(arg) = 1 / (2 * sqrt(arg))
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	return 1.0 / (2.0 * sqrt(arg));
}

//-------------------------------------------------------------------
//---- tanh
//-------------------------------------------------------------------

TanhFormula::TanhFormula () : UnaryFunctionFormula(FormulaName::Tanh)
{

}

double TanhFormula::EvalFunction (double arg)
{
	return tanh(arg);
}

double TanhFormula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = tanh(arg)
	//Jacobian multiplier: F'(arg) = 1 / cosh(arg)^2
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	double coshValue = cosh(arg);
	return 1.0 / (coshValue * coshValue);
}

//-------------------------------------------------------------------
//---- tan
//-------------------------------------------------------------------

TanFormula::TanFormula () : UnaryFunctionFormula(FormulaName::Tan)
{

}

double TanFormula::EvalFunction (double arg)
{
	return tan(arg);
}

double TanFormula::GetJacobianMultiplier (double arg)
{
	//Formula:             (R)     = F(arg) = tan(arg)
	//Jacobian multiplier: F'(arg) = 1/cos(arg)^2
	//(s. Comment in UnaryFunctionFormula::DE_Jacobian)
	
	double cosValue = cos(arg);
	return 1.0 / (cosValue * cosValue);
}



}//.. end "namespace SimModelNative"
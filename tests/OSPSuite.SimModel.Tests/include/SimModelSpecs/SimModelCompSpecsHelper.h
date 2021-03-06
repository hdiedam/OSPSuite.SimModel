#ifndef _SimModelCompSpecsHelper_H_
#define _SimModelCompSpecsHelper_H_

//forward declaration
class SimModelComp;

#include <string>
#include <vector>

namespace UnitTests
{
	class SimModelCompSpecsHelper
	{
	protected:
		::SimModelComp * _simModelComp;
	public:
		::SimModelComp * GetInstance();
		SimModelCompSpecsHelper(const char * simModelCompConfigFilePath);
		void ConfigureFrom(const char * schemaFilePath, const char * simulationFilePath);
		bool ProcessMetaData();
		bool ProcessData();
		virtual ~SimModelCompSpecsHelper();
		std::string DCILastError();
		void SetAllParametersAsVariable();
		void SetAllConstantParametersAsVariable();
		void SetParametersForCalculateSensitivity(std::vector<std::string> parameterPaths);

		std::vector<double> GetOutputTime();
		std::vector<double> GetOutputValues(const std::string & variableName);
		std::vector<double> GetOutputSensitivities(const std::string & variablePath, const std::string & parameterPath, 
			                                       const std::string & objectPathDelimiter);
		void SaveSimulationToXML(const std::string & file);

		std::vector<int> GetIds(const std::string & tableName);

		bool ContainsStringValue(const std::string & tableName,
			                     const std::string & columnName,
								 const std::string & value);
	};
}

#endif //_SimModelCompSpecsHelper_H_

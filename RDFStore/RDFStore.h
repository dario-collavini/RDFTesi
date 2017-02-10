#ifndef RDFSTORE_H_
#define RDFSTORE_H_

#include <CppRDFox/RDFoxDataStore.h>
#include "../Common/RDFStructures.h"
#include <algorithm>
#include <iostream>
#include <string.h>

/**
 * Singleton representing the main memory triple store RDFox.
 */
class RDFStore {
public:

	~RDFStore();

	static RDFStore* getInstance();

	/**
	 * Sets parameters for creating the RDFox store:
	 * one turtle file representing the ontology, the knowledge base;
	 * one file representing inference rules, a datalog file;
	 * one array and its length, for RDF prefixes: even positions for prefix, odd position for full URI
	 */
	void initialize(const char* type, const char* kb, const char* dlog, const char** prefixes, int prefixesLength);

	/**
	 * Adds a query that will be evaluated on the store during runtime execution.
	 * Queries are extracted from RDF Tesla rules.
	 */
	void addQuery(int eventType, const char* queryName, const char* queryString);

	/**
	 * Returns prefixes used in the store.
	 */
	const char** getPrefixesArray(){return prefixesArray;}

	int getPrefixesArrayLength(){return prefixesArrayLength;}

	std::map<int, RuleQuery*> &getQueryMap(){return queries;}

	/**
	 * Processes incoming events, importing a new one and deleting the old one,
	 * and evaluates stored queries after triple materialization.
	 * Queries results are stored as a vector of Events.
	 */
	std::vector<Event*> processRdfEvent(const char* event);

protected:

	RDFStore();

private:
	static RDFStore* instance;//singleton instance
	RDFoxDataStore *store;
	//Pre-processing parameters
	const char* type;
	const char* kb;
	const char* dlogRules;
	const char** prefixesArray;
	int prefixesArrayLength;
	//se voglio salvare copia db: const char* kbsave = "/home/dario/Scrivania/RDFoxProva/CppRDFox/file/provakb";
	// RDFoxDataStore_Save(RDFoxDataStore, const char* const fileName);
	std::map<int, RuleQuery*> queries;
	const char* eventOld;
	
	//variables used by RDFox for evaluating queries
	size_t* numVars;
	size_t* isQueryMatched;
	RDFoxDataStoreResourceID* resourceIDs;
	RDFoxDataStoreDatatypeID* datatypeID;
	char* resBuffer;						//result buffer
	size_t* bufferLength;
	RDFoxDataStoreTupleIterator* queryIterator;

	/**
	 * Evaluates all the queries saved in the store.
	 */
	std::vector<Event*> evaluateQueries();

	std::vector<Event*> evaluateSingleQuery(RuleQuery* q);
};
#endif /*RDFSTORE_H*/

#include "RDFStore.h"

//Max size of buffer length (full IRI)
const int LENGTH = 100;

RDFStore* RDFStore::instance = 0;
RDFStore* RDFStore::getInstance(){
	if(instance == 0)
		instance = new RDFStore;
	return instance;
}

RDFStore::RDFStore(){
	this->store = new RDFoxDataStore;
	this->numVars = new size_t;
	this->isQueryMatched = new size_t;
	this->resourceIDs = new RDFoxDataStoreResourceID;
	this->datatypeID = new RDFoxDataStoreDatatypeID;
	this->resBuffer = new char[LENGTH];
	this->bufferLength = new size_t;
	this->queryIterator = new RDFoxDataStoreTupleIterator;
	this->type = NULL;
	this->kb = NULL;
	this->dlogRules = NULL;
	this->prefixesArray = NULL;
	this->prefixesArrayLength = 0;
	this->eventOld = NULL;
}

RDFStore::~RDFStore(){
	delete this->store;
	delete this->numVars;
	delete this->isQueryMatched;
	delete this->resourceIDs;
	delete this->datatypeID;
	delete[] this->resBuffer;
	delete this->bufferLength;	
	delete this->queryIterator;
	delete this->type;
	delete this->kb;
	delete this->dlogRules;
	delete[] this->prefixesArray;
	for (std::map<int,RuleQuery*>::iterator query = this->queries.begin(); query != this->queries.end(); query++){	
		delete query->second;
	}
}

//Creates and initializes the RDFox store
void RDFStore::initialize(const char* type, const char* kb, const char* dlog, const char** prefixes, int prefixesLength){
	this->type = type;
	this->kb = kb;
	this->dlogRules = dlog;
	this->prefixesArray = prefixes;
	this->prefixesArrayLength = prefixesLength;
	RDFoxDataStore_Create(this->store, this->type, NULL , 0);
	RDFoxDataStore_Initialize(*(this->store));
	RDFoxDataStore_ImportFile(*(this->store), this->kb , 0, false);
	RDFoxDataStore_ImportFile(*(this->store), this->dlogRules , 0, false);
	RDFoxDataStore_ApplyRules(*(this->store), false);
}

//Finds var specified in the query string
std::vector<std::string> getVars(std::string query, int all){
	std::vector<std::string> var;
	std::string temp;
	int isNewVar = 1;

	for(std::string::iterator it=query.begin(); it!=query.end(); it++){
		if(*it == '?' || *it == '$'){//found var
			it++;
			while (*it != ' '){
				temp.push_back(*it);
				it++;
			}
			if(all == 1){//checks for duplicates (for 'select *' queries, where pattern must be checked to extract vars)
				for(std::vector<std::string>::iterator itVars=var.begin(); itVars!=var.end(); itVars++){
					if(temp.compare(*itVars) == 0){
						isNewVar = 0;
						break;//it's a duplicate, can exit
					}
				}
			}
			if(isNewVar == 1){
				var.push_back(temp);
			}
			isNewVar = 1;
			temp.clear();
		}
	}
	return var;
}

//RDFox "select *" queries not properly working...(?s ?p ?o -> resource[0]=?p, [1]=?o, [2]=?s )
//It is better to specify all vars separatedly
std::vector<std::string> findVars(const char* query){
	std::string temp = query;
	std::string querystring = query;
	unsigned inizio, fine;
	size_t found;
  
	std::transform(querystring.begin(), querystring.end(), temp.begin(), ::tolower);
	inizio = temp.find("select");
	fine = temp.find(" where");
	std::string nuova = querystring.substr(inizio+7, (fine-7)-inizio+1);
	found = nuova.find_first_of("*");
	if(found != std::string::npos){
		return getVars(querystring.substr(querystring.find_first_of("{"), querystring.find_last_of("}")-querystring.find_first_of("{")+1), 1);
	}else{
		return getVars(nuova, 0);
	}
}

void RDFStore::addQuery(int type, std::string name, std::string string){
	if(queries.find(type) == queries.end()){//add a new query if it is not present
		RuleQuery *temp = new RuleQuery;
		temp->eventType = type;
		temp->queryName = name;
		temp->queryString = string;
		temp->vars = findVars(string.c_str());
		queries.insert(std::pair<int, RuleQuery*>(type, temp));
	}
}

//Import update: 0 ADD
//               1 SCHEDULE_FOR_ADDITION
//		 		 2 SCHEDULE_FOR_DELETION
//See RDFox: Common.h
std::vector<Event*> RDFStore::processRdfEvent(const char* event){
	std::vector<Event*> teslaEvents;	
	if(this->eventOld != NULL){
		RDFoxDataStore_ImportFile(*(this->store), this->eventOld , 2, false);//delete last event
	}
	RDFoxDataStore_ImportFile(*(this->store), event , 1, false);//add new event
	RDFoxDataStore_ApplyRules(*(this->store), true); //true = incrementally
	this->eventOld = event;
	teslaEvents = evaluateQueries();
	return teslaEvents;
}

//Can be optimized by selecting only worthy queries (<--TRex could pass useful info about what to evaluate (event = query))
std::vector<Event*> RDFStore::evaluateQueries(){
	std::vector<Event*> allEvents;
	std::vector<Event*> singleQueryEvents;
	for (std::map<int,RuleQuery*>::iterator query = this->queries.begin(); query != this->queries.end(); query++){
		singleQueryEvents = evaluateSingleQuery(query->second);
		for(std::vector<Event*>::iterator it = singleQueryEvents.begin() ; it != singleQueryEvents.end(); it++){
			allEvents.push_back(*it);
		}
	}
	return allEvents;
}

//Calls RDFox API
std::vector<Event*> RDFStore::evaluateSingleQuery(RuleQuery* q){
	std::vector<Event*> events;
	Resource *tempRes;
	Event *tempEvent;								
	RDFoxDataStoreTupleIterator_CompileQuery(queryIterator,*(this->store), q->queryString.c_str(), NULL, 0, this->prefixesArray, this->prefixesArrayLength);
	RDFoxDataStoreTupleIterator_GetArity(this->numVars, *(this->queryIterator));
	RDFoxDataStoreTupleIterator_Open(this->isQueryMatched, *(this->queryIterator), *(this->numVars), this->resourceIDs);
	while(*(this->isQueryMatched) != 0){//while there are still  query results, do the loop and create a new event
		tempEvent = new Event;
		tempEvent->eventType = q->eventType;
		for(unsigned int j = 0; j < *(this->numVars); j++){//define attributes of event (results of query variables)
			tempRes = new Resource;
			tempRes->lexicalForm = new char[LENGTH];
			//double call, otherwise resBuffer is not properly filled (RDFox bug?)
			RDFoxDataStoreTupleIterator_GetResource(*queryIterator, resourceIDs[j], datatypeID, resBuffer, bufferLength);
			RDFoxDataStoreTupleIterator_GetResource(*queryIterator, resourceIDs[j], datatypeID, resBuffer, bufferLength);
			strcpy(tempRes->lexicalForm, this->resBuffer);
			tempRes->datatypeID = *(this->datatypeID);
			tempEvent->attributes.insert(std::pair<std::string, Resource*>(q->vars[j],tempRes));
		}
		events.push_back(tempEvent);
		RDFoxDataStoreTupleIterator_GetNext(this->isQueryMatched, *(this->queryIterator), *(this->numVars), this->resourceIDs);
	}
	return events;
}

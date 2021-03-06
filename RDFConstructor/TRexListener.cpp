#include "TRexListener.h"

//Max result length as defined in TRex
const int LEN = 16;

TRexListener::TRexListener(RDFConstructor* constructor){
	this->constructor = constructor;
}	

TRexListener::~TRexListener(){
}

//Converts the result string from RDFox into a compatible TRex value
std::string getValue(Attribute att){
	std::string result;
	switch(att.type){
		case INT:
			result = static_cast<std::ostringstream*>( &(std::ostringstream() << att.intVal) )->str();
			break;
		case FLOAT:
			result = static_cast<std::ostringstream*>( &(std::ostringstream() << att.floatVal) )->str();
			break;
		case BOOL:
			result = static_cast<std::ostringstream*>( &(std::ostringstream() << att.boolVal) )->str();
			break;
		case STRING:
			result = att.stringVal;
			break;
	}
	return result;
}

//Converts the PubPkt into an RDFEvent
RDFEvent* createRDF(PubPkt* pkt, Template* templateCE){
	RDFEvent* event = new RDFEvent;
	Attribute att;
	char* varName;
	std::map<std::string, Attribute> attributesMap;
	event->eventType = pkt->getEventType();
	event->prefixesArray = RDFStore::getInstance()->getPrefixesArray();
	event->prefixesArrayLength = RDFStore::getInstance()->getPrefixesArrayLength();
	for(std::vector<TripleTemplate>::iterator it =templateCE->triples.begin(); it != templateCE->triples.end(); it++){
		Triple t;
		t.subject = new char[LEN];
		t.predicate = new char[LEN];
		t.object = new char[LEN];
		strcpy(t.subject, it->subject.second.c_str());
		strcpy(t.predicate, it->predicate.second.c_str());
		strcpy(t.object, it->object.second.c_str());
		event->triples.push_back(t);
	}
	for(int k = 0; k < pkt->getAttributesNum(); k++){
		att = pkt->getAttribute(k);
		varName = att.name;
		attributesMap.insert(std::make_pair(varName, att)); //attributes saved for final evaluation of subscriptions
		for(unsigned int i = 0; i < templateCE->triples.size(); i++){
			TripleTemplate temp = templateCE->triples[i];
			if(temp.subject.first == IS_VAR && strcmp((temp.subject.second.c_str()+1), varName) == 0 ){//+1 removes the '?' or '$' of sparql var
				strcpy(event->triples[i].subject, getValue(att).c_str());
			}
			if(temp.predicate.first == IS_VAR && strcmp((temp.predicate.second.c_str()+1), varName) == 0 ){
				strcpy(event->triples[i].predicate, getValue(att).c_str());
			}
			if(temp.object.first == IS_VAR && strcmp((temp.object.second.c_str()+1), varName) == 0 ){
				strcpy(event->triples[i].object, getValue(att).c_str());
			}
		}
	}
	event->attributes.push_back(attributesMap);
	return event;
}

//Called just for ALL_WITHIN rules
std::vector<RDFEvent*> createRDFAll(std::map<int, std::vector<PubPkt*> > typesOfGroupEvents, std::map<int, Template*> templates){
	std::vector<RDFEvent*> results;
	for(std::map<int, std::vector<PubPkt*> >::iterator it = typesOfGroupEvents.begin(); it != typesOfGroupEvents.end(); it++){
		int nEventsToCreate;//init
		int numAllEvents = 0;
		std::vector<PubPkt*> pubPktVector = it->second;
		Template* templateCE = templates.find(it->first)->second;
		unsigned int numOfTriples = templateCE->triples.size();
		unsigned int numOfPkt =  pubPktVector.size();
		if(templateCE->isRuleEachAllWithin == true){//workaround for rules with each-within and all-within together in the same pattern
			//Maintains the AllHelper structure defined in the template by deleting no more useful timestamps and counting number of events
			//it works for patterns like from A and each B from A and all C from B (simple each...all patterns), but for more complex ones?
			std::list<TimeMs>::iterator rootEvTime = templateCE->allRuleInfos->RootTimestamps.begin();
			std::list<TimeMs>::iterator allEvTime = templateCE->allRuleInfos->AllTimestamps.begin();
			bool deleted = false;
			//clean root timestamps no more useful
			while(rootEvTime != templateCE->allRuleInfos->RootTimestamps.end()){
				if(rootEvTime->getTimeVal() <= allEvTime->getTimeVal()){
					//terminator event happened before the all event, don't need this value anymore
					deleted = true;
					rootEvTime = templateCE->allRuleInfos->RootTimestamps.erase(rootEvTime);//side-effect: increase rootEvTime
				}else{
					//found root > allEvent (c'è per forza perchè è stato creato almeno un evento complesso)
					break;//found, can exit
				}
				if(deleted == false){//increment (if rootTimestamp has not been deleted)
					rootEvTime++;	//actually it never enters it (timeRoot is always found because a Complex Event has been created, but when it is found it exits thanks to break instruction)
				}
				deleted = false;
			}
			//loop on AllEvents, deleting the ones no more valid (out of window size)
			while(allEvTime != templateCE->allRuleInfos->AllTimestamps.end()){
				if((rootEvTime->getTimeVal() - allEvTime->getTimeVal()) > templateCE->allRuleInfos->window.getTimeVal()){
					//window root/event > than max, can delete the element (no more useful)
					allEvTime = templateCE->allRuleInfos->AllTimestamps.erase(allEvTime);
				}else if(rootEvTime->getTimeVal() - allEvTime->getTimeVal() < 0){
					//can exit, because allEvent > rootEvent
					templateCE->allRuleInfos->RootTimestamps.pop_front();//delete first element, we don't need it anymore
					break;
				}
				else{
					numAllEvents++;//count the number of "all" events for each "each"
					allEvTime++;
				}
			}
			nEventsToCreate = (numOfPkt / numAllEvents);			//one event for each "each"
		}else{
			nEventsToCreate = 1;									//one single event
			numAllEvents = numOfPkt;
		}
		for(int e = 0; e < nEventsToCreate; e++){
			RDFEvent* event = new RDFEvent;
			event->eventType = it->first;
			event->prefixesArray = RDFStore::getInstance()->getPrefixesArray();
			event->prefixesArrayLength = RDFStore::getInstance()->getPrefixesArrayLength();
			for(int j = 0 + e*numAllEvents; j < numAllEvents + e*numAllEvents; j++){
				PubPkt* pubPkt = pubPktVector[j];
				std::map<std::string, Attribute> attributesMap;
				std::vector<Triple> dupTriples;
				for(unsigned int n = 0; n < numOfTriples; n++){
					TripleTemplate temp = templateCE->triples[n];
					int index;
					ValType attType;
					Attribute att;
					if(j == (0 + e*numAllEvents)){//first PubPkt, no duplicates for sure
						Triple t;
						t.subject = new char[LEN];
						t.predicate = new char[LEN];
						t.object = new char[LEN];
						strcpy(t.subject, temp.subject.second.c_str());
						strcpy(t.predicate,temp.predicate.second.c_str());
						strcpy(t.object, temp.object.second.c_str());
						if(temp.subject.first == IS_VAR){
							pubPkt->getAttributeIndexAndType(t.subject+1, index, attType);//+1 removes '?' or '$' of sparql var
							att = pubPkt->getAttribute(index);
							attributesMap.insert(std::make_pair(att.name, att));//attributes saved for final evaluation of subscriptions
							strcpy(t.subject, getValue(att).c_str());
						}
						if(temp.predicate.first == IS_VAR){
							pubPkt->getAttributeIndexAndType(t.predicate+1, index, attType);
							att = pubPkt->getAttribute(index);
							attributesMap.insert(std::make_pair(att.name, att));
							strcpy(t.predicate, getValue(att).c_str());
						}
						if(temp.object.first == IS_VAR){
							pubPkt->getAttributeIndexAndType(t.object+1, index, attType);
							att = pubPkt->getAttribute(index);
							attributesMap.insert(std::make_pair(att.name, att));
							strcpy(t.object, getValue(att).c_str());
						}
						event->triples.push_back(t);
					}else{//it is not the first PubPkt, needs to be handled as a duplicate
						Triple dt;
						dt.subject = new char[LEN];
						dt.predicate = new char[LEN];
						dt.object = new char[LEN];
						strcpy(dt.subject, temp.subject.second.c_str());
						strcpy(dt.predicate,temp.predicate.second.c_str());
						strcpy(dt.object, temp.object.second.c_str());
						if(temp.subject.first == IS_VAR){
							pubPkt->getAttributeIndexAndType(dt.subject+1, index, attType);
							att = pubPkt->getAttribute(index);
							attributesMap.insert(std::make_pair(att.name, att));
							strcpy(dt.subject, getValue(att).c_str());
						}
						if(temp.predicate.first == IS_VAR){
							pubPkt->getAttributeIndexAndType(dt.predicate+1, index, attType);
							att = pubPkt->getAttribute(index);
							attributesMap.insert(std::make_pair(att.name, att));
							strcpy(dt.predicate, getValue(att).c_str());
						}
						if(temp.object.first == IS_VAR){
							pubPkt->getAttributeIndexAndType(dt.object+1, index, attType);
							att = pubPkt->getAttribute(index);
							attributesMap.insert(std::make_pair(att.name, att));
							strcpy(dt.object, getValue(att).c_str());
						}
						dupTriples.push_back(dt);
					}
				}
				if(j != (0 + e*numAllEvents)) event->duplicateTriples.push_back(dupTriples);//it's not first PubPkt, so push duplicate PubPkt into the vector
				event->attributes.push_back(attributesMap);//vector[0] is for first PubPkt attributes, >0 for duplicates PubPkt attributes
			}
			results.push_back(event);
		}
	}
	return results;
}


//notifies listeners saved in the RDF Constructor
void TRexListener::notifyRDFListeners(RDFEvent* event){
	for(std::set<RDFResultListener*>::iterator it=constructor->getRDFListeners().begin(); it!=constructor->getRDFListeners().end(); it++) {
		RDFResultListener *listener = *it;
		listener->handleResult(event);
	}
}

//Implements the lifting rule, converting PubPkts into RDFEvents, and notifies rdf output listeners
void TRexListener::handleResult(std::set<PubPkt *> &genPkts, double procTime){
	bool atLeastOneAll = false;
	std::map<int, std::vector<PubPkt*> > typesOfGroupEvents;
	std::map<int, Template*> templates = this->constructor->getRdfEventTemplates();
	for (std::set<PubPkt*>::iterator i= genPkts.begin(); i != genPkts.end(); i++){
		PubPkt* pubPkt= *i;
		int type = pubPkt->getEventType();
		Template* templateCE = templates.find(type)->second;
		if(templateCE->isRuleAllWithin == true){
			atLeastOneAll = true;
			std::map<int, std::vector<PubPkt*> >::iterator it = typesOfGroupEvents.find(type);
			if(it == typesOfGroupEvents.end()){
				//new complex event(=generated pkt) type, adds a vector for this type
				std::vector<PubPkt*> eventsToGroup;
				eventsToGroup.push_back(pubPkt);
				typesOfGroupEvents.insert(std::pair<int, std::vector<PubPkt*> >(type, eventsToGroup));
			}else{
				//type already defined, adds the PubPkt in the existing vector
				it->second.push_back(pubPkt);
			}
		}else{//rule is not ALL_WITHIN
			//generate RDF for single PubPkt
			RDFEvent *rdfEvent = createRDF(pubPkt, templateCE);
			this->notifyRDFListeners(rdfEvent);
			//notification done, freeing memory...
			for(std::vector<Triple>::iterator it = rdfEvent->triples.begin(); it != rdfEvent->triples.end(); it++){
				delete it->subject;
				delete it->predicate;
				delete it->object;
			}
			delete rdfEvent;
		}
	}
	if(atLeastOneAll == true){//the rule has ALL_WITHIN, needs to call another function
	   std::vector<RDFEvent*> rdfEventsAll = createRDFAll(typesOfGroupEvents, templates);
	   for(std::vector<RDFEvent*>::iterator event = rdfEventsAll.begin(); event != rdfEventsAll.end(); event++){
		   this->notifyRDFListeners(*event);
		   //free memory of triples and duplicates
		   for(std::vector<Triple>::iterator triple = (*event)->triples.begin(); triple != (*event)->triples.end(); triple++){
			   delete triple->subject;
		   	   delete triple->predicate;
		   	   delete triple->object;
		   }
		   for(std::vector<std::vector<Triple> >::iterator dupTripleVector = (*event)->duplicateTriples.begin(); dupTripleVector != (*event)->duplicateTriples.end(); dupTripleVector++){
			   for(std::vector<Triple>::iterator dupTriple = dupTripleVector->begin(); dupTriple != dupTripleVector->end(); dupTriple++){
				   delete dupTriple->subject;
		  		   delete dupTriple->predicate;
		  		   delete dupTriple->object;
			   }
		   }
		   delete *event;
	   }
	}
}

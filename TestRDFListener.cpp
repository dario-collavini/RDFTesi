#include "./TestRDFListener.h"

static int lastId= 0;

TestRDFListener::TestRDFListener(SubPkt* subscription):
	subscription(subscription), id(lastId++)
{}

TestRDFListener::~TestRDFListener(){
	delete subscription;
}

bool matches(const Constraint constr, const Attribute attr){
	if (strcmp(constr.name, attr.name)!=0) return false;
	if (constr.type != attr.type) return false;
	switch (constr.type){
	case INT:
		switch (constr.op) {
		case EQ:
			if (attr.intVal!=constr.intVal) return false;
			break;
		case LT:
			if (attr.intVal>=constr.intVal) return false;
			break;
		case GT:
			if (attr.intVal<=constr.intVal) return false;
			break;
		case LE:
			if (attr.intVal>constr.intVal) return false;
			break;
		case GE:
			if (attr.intVal<constr.intVal) return false;
			break;
		
		case NE:
			if (attr.intVal==constr.intVal) return false;
			break;
		case IN:
			throw std::invalid_argument("IN operation cannot be used with INT type");
			break;
		}
		break;
	case FLOAT:
		switch (constr.op) {
		case EQ:
			if (attr.floatVal!=constr.floatVal) return false;
			break;
		case LT:
			if (attr.floatVal>=constr.floatVal) return false;
			break;
		case GT:
			if (attr.floatVal<=constr.floatVal) return false;
			break;
		case LE:
			if (attr.floatVal>constr.floatVal) return false;
			break;
		case GE:
			if (attr.floatVal<constr.floatVal) return false;
			break;

		case NE:
			if (attr.floatVal==constr.floatVal) return false;
			break;
		case IN:
			throw std::invalid_argument("IN operation cannot be used with FLOAT type");
			break;
		}
		break;
	case BOOL:
		switch (constr.op) {
		case EQ:
			if (attr.boolVal!=constr.boolVal) return false;
			break;
		case LT:
			throw std::invalid_argument("LT operation cannot be used with BOOL type");
			break;
		case GT:
			throw std::invalid_argument("GT operation cannot be used with BOOL type");
			break;
		case LE:
			throw std::invalid_argument("LE operation cannot be used with BOOL type");
			break;
		case GE:
			throw std::invalid_argument("GE operation cannot be used with BOOL type");
			break;

		case NE:
			if (attr.boolVal==constr.boolVal) return false;
			break;
		case IN:
			throw std::invalid_argument("IN operation cannot be used with BOOL type");
			break;
		}
		break;
	case STRING:
		switch (constr.op) {
		case EQ:
			if (strcmp(attr.stringVal, constr.stringVal)!=0) return false;
			break;
		case LT:
			if (strcmp(attr.stringVal, constr.stringVal)>=0) return false;
			break;
		case GT:
			if (strcmp(attr.stringVal, constr.stringVal)<=0) return false;
			break;
		case NE:
			if (strcmp(attr.stringVal, constr.stringVal)==0) return false;
			break;
		case LE:
			if (strcmp(attr.stringVal, constr.stringVal)>0) return false;
			break;
		case GE:
			if (strcmp(attr.stringVal, constr.stringVal)<0) return false;
			break;
			
		case IN:
			// The constraint's value should be a substring of the attribute's value:
			// it is a filter specified for published events' attributes
			if (strstr(attr.stringVal, constr.stringVal)==NULL) return false;
			break;
		}
		break;
	}

	return true;
}

//Checks match of the first event
bool matches(SubPkt* sub, RDFEvent* event){
	if (event->eventType!=sub->getEventType()) return false;
	for (int i=0; i<sub->getConstraintsNum(); i++){
		Constraint constr= sub->getConstraint(i);
		if(strcmp(event->attributes[0].find(constr.name)->second.name, constr.name) != 0) return false;
		if(!matches(constr, event->attributes[0].find(constr.name)->second)) return false;
	}
	return true;
}

//If rule is of type ALL, checks match for duplicates event and print just the ones satisfying subscription constraints
bool matchesDuplicate(SubPkt* sub, RDFEvent* event, unsigned int n){
	if(event->eventType!=sub->getEventType()) return false;
	if(event->attributes[n].empty()) return true;//map empty = event without vars, can just print duplicate triples without vars
	for (int i=0; i<sub->getConstraintsNum(); i++){
		Constraint constr= sub->getConstraint(i);
		if(strcmp(event->attributes[n].find(constr.name)->second.name, constr.name) != 0) return false;
		if(!matches(constr, event->attributes[n].find(constr.name)->second)) return false;
	}
	return true;
}


//For further processing of RDFEvent, a copy must be done (event memory is freed by TRexListener)
void TestRDFListener::handleResult(RDFEvent* event){
	bool isFirstEvent = true;
	if(matches(subscription, event)){
		printMessage("New complex event created:");
		for(int n = 0; n < event->prefixesArrayLength; n = n+2){
			std::cout << "@prefix " << event->prefixesArray[n] << " <" << event->prefixesArray[n+1] << "> ." << std::endl;
		}
		std::cout <<"{";
		for(std::vector<Triple>::iterator triple = event->triples.begin();triple != event->triples.end();triple++){
			std::cout << triple->subject << " ";
			std::cout << triple->predicate << " ";
			std::cout << triple->object;
			if(triple < event->triples.end()-1) std::cout << ".\n";
			else std::cout << ".";
		}
		std::cout << "}\n";
		isFirstEvent = false;
	}
	for(unsigned int i = 0; i < event->duplicateTriples.size(); i++){
		std::vector<Triple> vector = event->duplicateTriples[i];
		if(matchesDuplicate(subscription, event, i+1)){//+1: index for duplicated events starts from 1 (attributes[0] is for the first event)
			if(isFirstEvent){
				printMessage("New complex event created:");
			}
			isFirstEvent = false;
			std::cout <<"{";
			for(std::vector<Triple>::iterator dupTriple = vector.begin(); dupTriple != vector.end(); dupTriple++){
				std::cout << dupTriple->subject << " ";
				std::cout << dupTriple->predicate << " ";
				std::cout << dupTriple->object;
				if(dupTriple < vector.end()-1)
					std::cout << ".\n";
				else
					std::cout << ".";
			}
			std::cout << "}\n";
		}
	}
}

void TestRDFListener::printMessage(std::string msg){
	std::cout << "TestRDFListener" << id << " > " << msg << std::endl;
}

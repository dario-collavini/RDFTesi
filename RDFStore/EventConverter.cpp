#include "EventConverter.h"

//Substitutes the first part of an IRI with its prefix, as defined in the RDF store
void shrinkIRI(char* lexicalForm, const char** prefixesArray, int prefixesLength){
	std::string string(lexicalForm);
	size_t found;
	for(int i = 0; i < prefixesLength; i = i+2){
		std::string prefix(*(prefixesArray+i+1));
		found = string.find(prefix);
		if(found == 0){//the position of the prefix IRI must be the first character
			string.replace(string.begin(), string.begin()+ prefix.size(), *(prefixesArray+i));
			strcpy(lexicalForm, string.c_str());
			break;
		}
	}
}

//See RDFox: Common.h for datatypeID
//    TRex: Const.h for ValType
void defineAttFromType(Attribute* att, Resource* res, Event* event){
	switch(res->datatypeID){
		case 1://IRI
			att->type = STRING;
			shrinkIRI(res->lexicalForm, RDFStore::getInstance()->getPrefixesArray(), RDFStore::getInstance()->getPrefixesArrayLength());//cambio IRI col suo prefisso
			strcpy(att->stringVal, res->lexicalForm);
			break;
		case 5://INT
			att->type = INT;
			att->intVal = atoi(res->lexicalForm);
			break;
		case 6://FLOAT
			att->type = FLOAT;
			att->floatVal = (float)atof(res->lexicalForm);
			break;
		case 7://DOUBLE, loses precision
			att->type = FLOAT;
			att->floatVal = (float)atof(res->lexicalForm);
			break;
		case 8://BOOL
			att->type = BOOL;
			if(strcmp(res->lexicalForm, "true") == 0){
				att->boolVal = true;
			}else{
				att->boolVal = false;
			}
			break;
		default:
			att->type = STRING;
			strcpy(att->stringVal, res->lexicalForm);
			break;
	}
}

//Converts RdfEvent into TeslaEvent and frees memory about RdfEvent
std::vector<PubPkt*> EventConverter::convertToTesla(std::vector<Event*> events, RDFConstructor* constructor){
	std::vector<PubPkt*> pubPkts;
	std::map<int, Template*> templateMap = constructor->getRdfEventTemplates();
	for(std::vector<Event*>::iterator event = events.begin() ; event != events.end(); event++){
		int num = (*event)->attributes.size();
		int type = (*event)->eventType;
		Attribute tempAtt[num];
		int i = 0;
		for(std::map<std::string, Resource*>::iterator res = (*event)->attributes.begin(); res != (*event)->attributes.end(); res++){
			strcpy(tempAtt[i].name, res->first.c_str());
			defineAttFromType(tempAtt+i, res->second, *event);
			delete[] res->second->lexicalForm;
			delete res->second;//PubPkt created, frees event memory (no more needed)
			i++;
		}
		PubPkt* temp= new PubPkt(type, tempAtt, num);
		for(std::map<int, Template*>::iterator it = templateMap.begin(); it != templateMap.end(); it++){
			Template* templateCE = it->second;
			if(templateCE->isRuleAllWithin == true){
				if(type == templateCE->allRuleInfos->typeTerminator){
					templateCE->allRuleInfos->RootTimestamps.push_back(temp->getTimeStamp());
				}
				if(type == templateCE->allRuleInfos->typeEventAll){
					templateCE->allRuleInfos->AllTimestamps.push_back(temp->getTimeStamp());
				}
			}
		}
		pubPkts.push_back(temp);
		delete *event;
	}
	return pubPkts;	
}

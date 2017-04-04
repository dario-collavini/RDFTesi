#include <TRex2/Engine/TRexEngine.h>
#include <TRex2/Engine/ResultListener.h>
#include <TRex2/Packets/SubPkt.h>
#include <TRex2/Packets/PubPkt.h>
#include "TestRDFListener.h"
#include "RDFStore/RDFStore.h"
#include "RDFStore/EventConverter.h"
#include "Parser/RDFTRexRuleParser.h"
#include "RDFConstructor/RDFConstructor.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <queue>

const int maxSize = 100;
std::queue<const char*> inputQueue;
int a = 1;//For print purposes

void processPubPkt(vector<PubPkt*> pubPkts, TRexEngine* e){
	if(pubPkts.size() == 0) std::cout << "\n\n\n";
	for (vector<PubPkt*>::iterator it= pubPkts.begin(); it != pubPkts.end(); it++){
		std::cout << "...processo pacchetto..." << "\n";
		e->processPubPkt(*it);
	}
	a++;
}

int main(int argc, char* argv[])
{
/*Pre-processing: setting di RDFox e TRex da parte di un server dedicato.
*	RDFox:  		1)creo e inizializzo store con parametri passati;
*					2)import triple ontologia;
*					3)import regole datalog ontologia;
*					4)materializzazione triple (esplicite(=caricate) + implicite(=dedotte da regole)).
*		
*	TRex:			5)inizializzo engine.
*
*	EventConverter: 6)implementa traduzione eventoRDF ----> eventoTesla (lowering rule) 
*/

	const char* type = "seq"; //cfr DataStore.java di RDFox per altri tipi (par-simple-nn, par-simple-nw, par-simple-ww, par-complex-nn, par-complex-nw, par-complex-ww)
	const char* kb = "./Files/kbradio.ttl";
	const char* dlogRules = "./Files/inf.dlog";
	int prefixesArrayLength = 4;
	const char* prefixesArray[prefixesArrayLength] = {"ad:", "http://my.org#", "a1:", "http://localhost#"};

	RDFStore* store = RDFStore::getInstance();
	store->initialize(type, kb, dlogRules, prefixesArray, prefixesArrayLength);
	TRexEngine* e = new TRexEngine(1);
	RDFConstructor* constructor= new RDFConstructor();
	EventConverter conv;

	e->finalize();
	//Subscription all'RDF Constructor, che farà da intermediario tra Trex e il vero listener(client), per produrre l'evento complesso RDF
	e->addResultListener(constructor->getTRexListener());
	
/*Subscription: il client passa subscription e regole.
*		Le subscription sono salvate nell'RDFConstructor, mentre un parser traduce le regole in regole TRex ed estrae le query passandole
*		a RDFox. Passa inoltre il template RDF dell'evento complesso all'RDF Constructor, incaricato di produrre l'evento RDF 
*		corrispondente quando viene generato il CE Tesla da TRex.
*/

//Parser
	RDFTRexRuleParser parser;
	std::ifstream regola("./teslardf_rule.txt");
	std::stringstream buffer;
	buffer << regola.rdbuf();
	std::string str = buffer.str();
	parser.parse(str, store, e, constructor);

//Subscription
	SubPkt *sub = new SubPkt(3);//3 è l'ID assegnato all'evento complesso nella regola SCEP (cfr. "teslardf_rule.txt")
	RDFResultListener *listener = new TestRDFListener(sub);
	constructor->addRDFListener(listener);

/*Runtime: arrivano eventi al server (pubblicazioni da parte del client).
*	   1)materializzo in kb.
*	   2)valuto query.
*	   3)passo risultati al Result converter (lowering rule)
*	   4)eventi convertiti mandati in input a TRex
*	   5)TRex fa processing e manda risultati al listener<---RDF Constructor che ricevendoli è incaricato di produrre il CE RDF come output ai    *            vari RDF listener
*/	
	const char* e1 = "./TestEvents/t1.ttl";
	const char* e2 = "./TestEvents/t2.ttl";
	const char* e3 = "./TestEvents/t3.ttl";
	const char* e4 = "./TestEvents/t4.ttl";
	const char* e5 = "./TestEvents/t6.ttl";
	const char* e6 = "./TestEvents/t7.ttl";
	const char* e7 = "./TestEvents/t8.ttl";
	
	if(inputQueue.size() < maxSize) inputQueue.push(e1);
	if(inputQueue.size() < maxSize) inputQueue.push(e2);
	if(inputQueue.size() < maxSize) inputQueue.push(e3);
	if(inputQueue.size() < maxSize) inputQueue.push(e4);
	if(inputQueue.size() < maxSize) inputQueue.push(e5);
	if(inputQueue.size() < maxSize) inputQueue.push(e6);
	if(inputQueue.size() < maxSize) inputQueue.push(e7);


	while(!inputQueue.empty()){
		std::cout << "--------------------Evento " << a << "-----------------------------" << "\n";
		const char* RDFevent = inputQueue.front();
		inputQueue.pop();
		std::vector<Event*> events = store->processRdfEvent(RDFevent);
		std::vector<PubPkt*> teslaEvents = conv.convertToTesla(events, constructor);
		processPubPkt(teslaEvents, e);
	}
	std::cout << "-------------------------------------------------------------------" << "\n";

	return 0;
}

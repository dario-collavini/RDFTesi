#ifndef RDFSTRUCTURES_H
#define RDFSTRUCTURES_H

#include <TRex2/Common/Structures.h>
#include <vector>
#include <map>
#include <string>

/**
 * Used inside RDF complex event template
 * to know if a triple sub/pred/obj are vars.
 */
enum IsVar{
	IS_NOT_VAR = 0,
	IS_VAR = 1
};

/**
 * Represents a query extracted from parsing a RDF Tesla rule.
 */
typedef struct SparqlQuery {
	int eventType;
	const char* queryName;
	const char* queryString;
	std::vector<std::string> vars;
} RuleQuery;

/**
 * Represents one single variable result of a sparql query evaluated by the rdf store.
 * The value is saved inside a char buffer.
 * Noteworthy RDFox values compatible with TRex:
 * D_IRI_REFERENCE = 1;
 * D_XSD_STRING = 3;
 * D_XSD_INTEGER = 5;
 * D_XSD_FLOAT = 6;
 * D_XSD_DOUBLE = 7;
 * D_XSD_BOOLEAN = 8.
 */
typedef struct SparqlResource {
	int datatypeID; 	   //resource type as specified inside RDFox file Common.h
	char* lexicalForm;
} Resource;

/**
 * A collection of query results evaluated by the rdf store.
 * A query produces bindings between variable name and its actual value, and they are saved in a map.
 * As defined by the lowering rule, a query answer defines a Tesla event where sparql variables are its attributes.
 */
typedef struct SparqlAnswer {
	int eventType;				 					//event type == query type (as defined by the lowering rule)
	std::map<std::string, Resource*> attributes;
} Event;

/**
 * Single triple pattern of a RDF complex event, extracted from parsing a rule.
 */
typedef struct RDFTripleTemplate {
	std::pair<IsVar, std::string> subject;
	std::pair<IsVar, std::string> predicate;
	std::pair<IsVar, std::string> object;
} TripleTemplate;

/**
 * Pattern of a RDF complex event, extracted from parsing a rule.
 */
typedef struct RDFTemplate {
	int eventType;
	std::vector<TripleTemplate> triples;
	bool isRuleAllWithin;					//helps implementing a first version of ALL_WITHIN (without modifying TRex library)
} Template;

/**
 * A single triple result, part of a RDF Complex Event.
 */
typedef struct RDFTriple {
	char* subject;
	char* predicate;
	char* object;
} Triple;

/**
 * RDF Complex event, final result of RDF reasoning and CEP processing.
 * It is the RDF Complex event template, with TRex processing results instead of vars, as defined by the lifting rule.
 */
typedef struct ComplexEvent {
	int eventType;
	const char** prefixesArray; 								//RDF prefixes used for IRI (from RDF store)
	int prefixesArrayLength;
	std::vector<Triple> triples;								//triples of the first result event
	std::vector<std::vector<Triple> > duplicateTriples;			//duplicate triples as events of ALL_WITHIN rules
	std::vector<std::map<std::string, Attribute> > attributes; 	//for checking subscription constraints (0 is first event index; >0 index of duplicate events)
} RDFEvent;

#endif /*RDFSTRUCTURES_H*/

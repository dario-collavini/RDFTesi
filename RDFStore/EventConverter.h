#ifndef EVENTCONVERTER_H
#define EVENTCONVERTER_H

#include <TRex2/Packets/PubPkt.h>
#include "RDFStore.h"
#include "../Common/RDFStructures.h"
#include <vector>
#include <string>

/**
 * Class that implements the lowering rule.
 * It is just a function, kept separated from the RDF store,
 * just to emphasize the implementation of the lowering rule.
 */
class EventConverter {
public:
	/**
	 * Takes as input the result of sparql queries over the store, as a vector of Events,
	 * and convert it into a vector of PubPkts, that will be the TRex engine input.
	 */
	std::vector<PubPkt*> convertToTesla(std::vector<Event*> events);
};

#endif /*EVENTCONVERTER_H*/

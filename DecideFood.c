#include "Track.h"

FEEDER_STATE DecideFood(unsigned long Time, BOOL bInPreferenceSector, PPInfo *pp)
{
	// out<=>in_wait->in_feed->in_fed<=>out_refractory->out		
	if (bInPreferenceSector) { // in


		if (pp->m_Food == out) {	//out -> in_wait
			pp->m_Food = in_wait;
			pp->m_StopWait = Time + pp->m_FeederEntranceLatency;
		} else if (pp->m_Food == in_wait && Time >= pp->m_StopWait) { // in_wait -> in_feed
			pp->m_Food = in_feed;
			pp->m_NumberOfEntrances++;
		} else if (pp->m_Food == in_feed) { // in_feed -> in_fed
			pp->m_Food = in_fed;
			pp->m_StopRefractory = Time + pp->m_FeederExitLatency;
		} else if (pp->m_Food == out_refractory) {	// out_refractory -> in_fed
			pp->m_Food = in_fed;
		}

	} else { // out
		if (pp->m_Food == in_wait) { // in_wait -> out
			pp->m_Food = out;
		} else if (pp->m_Food == in_feed) { // in_feed -> (in_fed) -> out_refractory 
			pp->m_Food = out_refractory;
			pp->m_StopRefractory = Time + pp->m_FeederExitLatency;
		} else if (pp->m_Food == in_fed) {	// in_fed -> out_refractory
			pp->m_Food = out_refractory;
		} else if (pp->m_Food == out_refractory && Time >= pp->m_StopRefractory) {
			pp->m_Food = out;	// out_refractory -> out
		}
	}
	return (pp->m_Food);
}

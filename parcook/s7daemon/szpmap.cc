#include "szpmap.h"

SzParamMap::SzParam SzParamMap::ConfigureNode(unsigned long int idx, TAttribHolder *node)
{	
	std::string val_type = node->getAttribute<std::string>("extra:val_type");
	std::string val_op = node->getAttribute<std::string>("extra:val_op", "");
	int address = node->getAttribute<int>("extra:address");
	return SzParam(address, val_type, val_op, 0);
}

void SzParamMap::ConfigureSend(unsigned long int idx, SendParamInfo *s)
{	
	SzParam szp = ConfigureNode(idx, s);
	if (!szp.isValid())
		throw std::runtime_error("Could not configure send! Check previous logs!");

	if (auto min_val = s->getOptAttribute<double>("extra:min")) {
		szp.setMin(*min_val);
		szp.setHasMin(true);
	}

	if (auto max_val = s->getOptAttribute<double>("extra:max")) {
		szp.setMax(*max_val);
		szp.setHasMax(true);
	}

	sz_log(7, "SzParam::min = %f", szp.getMin());
	sz_log(7, "SzParam::max = %f", szp.getMax());

	if (szp.getMin() > szp.getMax()) {
		throw std::runtime_error("Configuration error: min > max");
	}

	/* extra:prec is required for sending floats */
	if (auto prec = s->getOptAttribute<int>("extra:prec")) {
		szp.setPrec(*prec);
	} else if (szp.isFloat()) {
		throw std::runtime_error("No attribute prec (required for float) given for send");
	}

	_params[idx] = szp;
	_send_params[idx] = s;
}

void SzParamMap::ConfigureParam(unsigned long int idx, IPCParamInfo *p)
{	
	sz_log(10, "SzParamMap::ConfigureParamFromXml(TParam)");

	SzParam szp = ConfigureNode(idx, p);
	if (!szp.isValid())
		throw std::runtime_error("Could not configure param. Check previous logs.");
	
	szp.setPrec(p->GetPrec());

	_params[idx] = szp;
}

void SzParamMap::AddParam(unsigned long int idx, SzParam param) 
{
	_params[idx] = param;
}



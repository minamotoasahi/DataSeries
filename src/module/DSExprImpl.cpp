#include "DSExprImpl.hpp"

#include <boost/foreach.hpp>

#include "DSExprParse.hpp"

void DSExprImpl::Driver::doit(const std::string &str)
{
    startScanning(str);
    
    DSExprImpl::Parser parser(*this, scanner_state);
    int ret = parser.parse();
    INVARIANT(ret == 0 && expr != NULL, "parse failed");

    finishScanning();
}

DSExprImpl::Driver::~Driver()
{
    INVARIANT(scanner_state == NULL, "bad");
}

//////////////////////////////////////////////////////////////////////

DSExprImpl::ExprField::ExprField(ExtentSeries &series, const string &fieldname_)
{ 
    // Allow for almost arbitrary fieldnames through escaping...
    if (fieldname_.find('\\', 0) != string::npos) {
	string fixup;
	fixup.reserve(fieldname_.size());
	for(unsigned i=0; i<fieldname_.size(); ++i) {
	    if (fieldname_[i] == '\\') {
		++i;
		INVARIANT(i < fieldname_.size(), "missing escaped value");
	    }
	    fixup.push_back(fieldname_[i]);
	}
	field = GeneralField::create(NULL, series, fixup);
	fieldname = fixup;
    } else {
	field = GeneralField::create(NULL, series, fieldname_);
	fieldname = fieldname_;
    }
}

//////////////////////////////////////////////////////////////////////

DSExprImpl::ExprStrLiteral::ExprStrLiteral(const string &l)
{
    string fixup;
    SINVARIANT(l.size() > 2);
    SINVARIANT(l[0] == '\"');
    SINVARIANT(l[l.size() - 1] == '\"');
    fixup.reserve(l.size() - 2);
    for (unsigned i = 1; i < l.size() - 1; ++i) {
	char cc = l[i];
	if (cc == '\\') {
	    ++i;
	    cc = l[i];
	    INVARIANT(i < l.size(), "missing escaped value");
	    switch (cc) {
	    case '\\':
		fixup.push_back('\\');
		break;
	    case '\"':
		fixup.push_back('\"');
		break;
	    case 'f':
		fixup.push_back('\f');
		break;
	    case 'r':
		fixup.push_back('\r');
		break;
	    case 'n':
		fixup.push_back('\n');
		break;
	    case 'b':
		fixup.push_back('\b');
		break;
	    case 't':
		fixup.push_back('\t');
		break;
	    case 'v':
		fixup.push_back('\v');
		break;
	    default:
		FATAL_ERROR("unknown escape");
	    }
	} else {
	    fixup.push_back(cc);
	}
    }
    s = fixup;
}

//////////////////////////////////////////////////////////////////////

DSExprImpl::ExprFunctionApplication::~ExprFunctionApplication()
{
    BOOST_FOREACH(DSExpr *e, args) {
	delete e;
    }
    args.clear();
}

void DSExprImpl::ExprFunctionApplication::dump(ostream &out) {
    out << name;
    out << "(";
    out << " ";
    bool first = true;
    BOOST_FOREACH(DSExpr *e, args) {
	if (first) {
	    first = false;
	} else {
	    out << ", ";
	}
	e->dump(out);
    }
    out << " ";
    out << ")";
}

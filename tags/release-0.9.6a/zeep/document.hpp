//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_XML_DOCUMENT_H
#define SOAP_XML_DOCUMENT_H

#include "node.hpp"

namespace xml {

class document : public boost::noncopyable
{
  public:
					document(
						std::istream&		data);
					
					document(
						const std::string&	data);

					document(
						node_ptr			data);

	virtual			~document();

	node_ptr		root() const;

  private:
	struct document_imp*	impl;
};

std::ostream& operator<<(std::ostream& lhs, const document& rhs);

}

#endif

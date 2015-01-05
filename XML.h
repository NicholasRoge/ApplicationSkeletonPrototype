#ifndef XML_H
#define XML_H

#include <vector>


namespace XML
{
	class Element
	{
		public:
			enum class Type
			{
				COMMENT,
				CONTAINER,
				TEXT,
			};

		private:
			std::vector<Element> children;

		public:
			const Type type;

		public:
			Element(Type type);

			virtual void addChild();

			const std::vector<Element>& getChildren();
	};
}

#endif
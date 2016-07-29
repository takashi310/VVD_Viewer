#ifndef _BRKXML_WRITER_H_
#define _BRKXML_WRITER_H_

#include <vector>
#include <base_writer.h>
#include <tinyxml2.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>
#include <map>

class BRKXMLWriter : public BaseWriter
{
public:
	BRKXMLWriter();
	~BRKXMLWriter();

	void SetData(Nrrd* data);
	void SetSpacings(double spcx, double spcy, double spcz);
	void SetCompression(bool value);
	void Save(wstring filename, int mode);
	//save only a vvd_xml file with metadata
	void SaveVVDXML_Metadata(wstring filepath, tinyxml2::XMLDocument *vvd, tinyxml2::XMLDocument *metadata=NULL);
	//save only a vvd_xml file with external metadata
	void SaveVVDXML_ExternalMetadata(wstring filepath, tinyxml2::XMLDocument *vvd, tinyxml2::XMLDocument *metadata=NULL);
	void SaveMetadataXML(wstring filepath, tinyxml2::XMLDocument *metadata=NULL);
	void AddROITreeToMetadata(wstring roi_tree);

private:

	void buildROITreeXML(const boost::property_tree::wptree& tree, const map<int,vector<int>>& palette, const wstring& parent=wstring(), tinyxml2::XMLElement *lvNode=NULL);

	Nrrd* m_data;
	double m_spcx, m_spcy, m_spcz;
	bool m_use_spacings;
	bool m_compression;
	
	tinyxml2::XMLDocument m_doc;
	tinyxml2::XMLDocument m_md_doc;
};

#endif//_BRKXML_WRITER_H_
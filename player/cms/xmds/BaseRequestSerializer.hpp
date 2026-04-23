#pragma once

#include <sstream>
#include <string>
#include <string_view>

namespace Soap
{
    template <typename Request>
    class BaseRequestSerializer
    {
    public:
        BaseRequestSerializer(const Request& request) : m_request(request) {}

    protected:
        template <typename... Args>
        std::string createRequest(std::string_view requestName, Args... fields)
        {
            std::stringstream stream;
            stream
                << R"(<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:tns="urn:xmds" xmlns:types="urn:xmds/encodedTypes" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">)";
            stream << R"(<soap:Body soap:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">)";
            stream << "<tns:" << requestName << ">";
            ((stream << "<" << fields.name() << " xsi:type=\"xsd:" << fields.type() << "\">"
                     << escapeXmlValue(fields.value()) << "</" << fields.name() << ">"),
             ...);
            stream << "</tns:" << requestName << ">";
            stream << "</soap:Body>";
            stream << "</soap:Envelope>";

            return stream.str();
        }

        const Request& request() const
        {
            return m_request;
        }

    private:
        template <typename T>
        std::string escapeXmlValue(const T& value)
        {
            std::stringstream stream;
            stream << value;
            return escapeXmlString(stream.str());
        }

        std::string escapeXmlValue(const std::string& value)
        {
            return escapeXmlString(value);
        }

        std::string escapeXmlString(const std::string& value)
        {
            std::string escaped;
            escaped.reserve(value.size());

            for (char ch : value)
            {
                switch (ch)
                {
                    case '&': escaped += "&amp;"; break;
                    case '<': escaped += "&lt;"; break;
                    case '>': escaped += "&gt;"; break;
                    case '"': escaped += "&quot;"; break;
                    case '\'': escaped += "&apos;"; break;
                    default: escaped += ch; break;
                }
            }

            return escaped;
        }

        const Request& m_request;
    };
}

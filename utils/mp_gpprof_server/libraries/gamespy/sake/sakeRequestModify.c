///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "sakeRequestInternal.h"
#include "sakeRequest.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static SAKEStartRequestResult SAKE_CALL sakeiValidateRequestFields(SAKEField *fields, int numFields)
{
	int i;

	for(i = 0 ; i < numFields ; i++)
	{
		if(!fields[i].mName || !fields[i].mName[0])
			return SAKEStartRequestResult_BAD_FIELD_NAME;
		if((fields[i].mType >= SAKEFieldType_NUM_FIELD_TYPES))
			return SAKEStartRequestResult_BAD_FIELD_TYPE;
		if(fields[i].mType == SAKEFieldType_ASCII_STRING)
		{
			if(!fields[i].mValue.mAsciiString)
				return SAKEStartRequestResult_BAD_FIELD_VALUE;
		}
		if(fields[i].mType == SAKEFieldType_UNICODE_STRING)
		{
			if(!fields[i].mValue.mUnicodeString)
				return SAKEStartRequestResult_BAD_FIELD_VALUE;
		}
		if(fields[i].mType == SAKEFieldType_BINARY_DATA)
		{
			if(fields[i].mValue.mBinaryData.mLength < 0)
				return SAKEStartRequestResult_BAD_FIELD_VALUE;
			if(!fields[i].mValue.mBinaryData.mValue && (fields[i].mValue.mBinaryData.mLength > 0))
				return SAKEStartRequestResult_BAD_FIELD_VALUE;
		}
	}

	return SAKEStartRequestResult_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void SAKE_CALL sakeiFillSoapRequestFieldValues(SAKERequest request, SAKEField *fields, int numFields)
{
	int i;

	gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "values");

	for(i = 0 ; i < numFields ; i++)
	{
		gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "RecordField");
			gsXmlWriteStringElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "name", fields[i].mName);
			gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "value");

				// fill-in the type-specific value struct based on the type
				if(fields[i].mType == SAKEFieldType_BYTE)
				{
					gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "byteValue");
						gsXmlWriteIntElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "value", fields[i].mValue.mByte);
					gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "byteValue");
				}
				else if(fields[i].mType == SAKEFieldType_SHORT)
				{
					gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "shortValue");
						gsXmlWriteIntElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "value", fields[i].mValue.mShort);
					gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "shortValue");
				}
				else if(fields[i].mType == SAKEFieldType_INT)
				{
					gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "intValue");
						gsXmlWriteIntElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "value", (gsi_u32)fields[i].mValue.mInt);
					gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "intValue");
				}
				else if (fields[i].mType == SAKEFieldType_INT64)
				{
					gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "int64Value");
					gsXmlWriteInt64Element(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "value", fields[i].mValue.mInt64);
					gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "int64Value");
				}
				else if(fields[i].mType == SAKEFieldType_FLOAT)
				{
					gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "floatValue");
						gsXmlWriteFloatElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "value", fields[i].mValue.mFloat);
					gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "floatValue");
				}
				else if(fields[i].mType == SAKEFieldType_ASCII_STRING)
				{
					gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "asciiStringValue");
						gsXmlWriteStringElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "value", fields[i].mValue.mAsciiString);
					gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "asciiStringValue");
				}
				else if(fields[i].mType == SAKEFieldType_UNICODE_STRING)
				{
					gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "unicodeStringValue");
						gsXmlWriteUnicodeStringElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "value", fields[i].mValue.mUnicodeString);
					gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "unicodeStringValue");
				}
				else if(fields[i].mType == SAKEFieldType_BOOLEAN)
				{
					gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "booleanValue");
						gsXmlWriteIntElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "value", (gsi_u32)(gsi_is_false(fields[i].mValue.mBoolean)?0:1));
					gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "booleanValue");
				}
				else if(fields[i].mType == SAKEFieldType_DATE_AND_TIME)
				{
					gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "dateAndTimeValue");
						gsXmlWriteDateTimeElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "value", fields[i].mValue.mDateAndTime);
					gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "dateAndTimeValue");
				}
				else if(fields[i].mType == SAKEFieldType_BINARY_DATA)
				{
					gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "binaryDataValue");
						gsXmlWriteBase64BinaryElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "value",
							fields[i].mValue.mBinaryData.mValue, fields[i].mValue.mBinaryData.mLength);
					gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "binaryDataValue");
				}
				else
				{
					// a type isn't being handled
					GS_FAIL_STR("Unhandled field type");
				}
			gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "value");
		gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "RecordField");
	}

	gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "values");
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Create Record

static SAKEStartRequestResult SAKE_CALL sakeiCreateRecordValidateInput(SAKERequest request)
{
	SAKECreateRecordInput *input = (SAKECreateRecordInput *)request->mInput;

	// check the tableid
	if(!input->mTableId)
		return SAKEStartRequestResult_BAD_TABLEID;

	// check the num fields
	if(input->mNumFields < 0)
		return SAKEStartRequestResult_BAD_NUM_FIELDS;

	// check for NULL fields
	if(!input->mFields && (input->mNumFields > 0))
		return SAKEStartRequestResult_BAD_FIELDS;

	// check the fields
	return sakeiValidateRequestFields(input->mFields, input->mNumFields);
}

static SAKEStartRequestResult SAKE_CALL sakeiCreateRecordFillSoapRequest(SAKERequest request)
{
	SAKECreateRecordInput *input = (SAKECreateRecordInput *)request->mInput;

	// write the table id
	gsXmlWriteStringElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "tableid", input->mTableId);

	// fill in the field values
	sakeiFillSoapRequestFieldValues(request, input->mFields, input->mNumFields);

	return SAKEStartRequestResult_SUCCESS;
}

static SAKERequestResult sakeiCreateRecordProcessSoapResponse(SAKERequest request)
{
	SAKECreateRecordOutput *output = (SAKECreateRecordOutput *)request->mOutput;

	if(gsi_is_false(gsXmlReadChildAsInt(request->mSoapResponse, "recordid", &output->mRecordId)))
	{
		return SAKERequestResult_MALFORMED_RESPONSE;
	}

	return SAKERequestResult_SUCCESS;
}

SAKEStartRequestResult SAKE_CALL sakeiStartCreateRecordRequest(SAKERequest request)
{
	static SAKEIRequestInfo info =
	{
		sizeof(SAKECreateRecordOutput),
		SAKEI_FUNC_NAME_STRINGS("CreateRecord"),
		sakeiCreateRecordValidateInput,
		sakeiCreateRecordFillSoapRequest,
		sakeiCreateRecordProcessSoapResponse
	};

	return sakeiStartRequest(request, &info);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Update Record

static SAKEStartRequestResult SAKE_CALL sakeiUpdateRecordValidateInput(SAKERequest request)
{
	SAKEUpdateRecordInput *input = (SAKEUpdateRecordInput *)request->mInput;

	// check the tableid
	if(!input->mTableId)
		return SAKEStartRequestResult_BAD_TABLEID;

	// check the num fields
	if(input->mNumFields <= 0)
		return SAKEStartRequestResult_BAD_NUM_FIELDS;

	// check for NULL fields
	if(!input->mFields)
		return SAKEStartRequestResult_BAD_FIELDS;

	// check the fields
	return sakeiValidateRequestFields(input->mFields, input->mNumFields);
}

static SAKEStartRequestResult SAKE_CALL sakeiUpdateRecordFillSoapRequest(SAKERequest request)
{
	SAKEUpdateRecordInput *input = (SAKEUpdateRecordInput *)request->mInput;

	// write the table id
	gsXmlWriteStringElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "tableid", input->mTableId);

	// write the recordid
	gsXmlWriteIntElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "recordid", (gsi_u32)input->mRecordId);

	// fill in the field values
	sakeiFillSoapRequestFieldValues(request, input->mFields, input->mNumFields);

	return SAKEStartRequestResult_SUCCESS;
}

SAKEStartRequestResult SAKE_CALL sakeiStartUpdateRecordRequest(SAKERequest request)
{
	static SAKEIRequestInfo info =
	{
		0,
		SAKEI_FUNC_NAME_STRINGS("UpdateRecord"),
		sakeiUpdateRecordValidateInput,
		sakeiUpdateRecordFillSoapRequest,
		NULL
	};

	return sakeiStartRequest(request, &info);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Delete Record

static SAKEStartRequestResult SAKE_CALL sakeiDeleteRecordValidateInput(SAKERequest request)
{
	SAKEDeleteRecordInput *input = (SAKEDeleteRecordInput *)request->mInput;

	// check the tableid
	if(!input->mTableId)
		return SAKEStartRequestResult_BAD_TABLEID;

	return SAKEStartRequestResult_SUCCESS;
}

static SAKEStartRequestResult SAKE_CALL sakeiDeleteRecordFillSoapRequest(SAKERequest request)
{
	SAKEDeleteRecordInput *input = (SAKEDeleteRecordInput *)request->mInput;

	// write the table id
	gsXmlWriteStringElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "tableid", input->mTableId);

	// write the recordid
	gsXmlWriteIntElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "recordid", (gsi_u32)input->mRecordId);

	return SAKEStartRequestResult_SUCCESS;
}

SAKEStartRequestResult SAKE_CALL sakeiStartDeleteRecordRequest(SAKERequest request)
{
	static SAKEIRequestInfo info =
	{
		0,
		SAKEI_FUNC_NAME_STRINGS("DeleteRecord"),
		sakeiDeleteRecordValidateInput,
		sakeiDeleteRecordFillSoapRequest,
		NULL
	};

	return sakeiStartRequest(request, &info);
}

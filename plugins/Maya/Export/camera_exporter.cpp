#include "stdafx.h"
#include "camera_exporter.h"
#include "..\..\..\editors\Ecore\editor\EditObject.h"



bool CXRayCameraExport::haveReadMethod () const
{
    return false;
}

bool CXRayCameraExport::haveWriteMethod () const
{
	std::cerr <<"Dbg : bool CXRayCameraExport::haveWriteMethod () const\n";
    return true;
}

void* CXRayCameraExport::creator()
{
	return new CXRayCameraExport();
}

MStatus CXRayCameraExport::reader(const MFileObject& file, const MString& optionsString, FileAccessMode mode)
{
    fprintf(stderr, "CXRayCameraExport::reader called in error\n");
    return MS::kFailure;
}

MStatus CXRayCameraExport::writer(const MFileObject& file, const MString& optionsString, FileAccessMode mode)
{
    return ExportCamera(file);
}

MString CXRayCameraExport::filter()const
{
	std::cerr <<"Dbg : MString CXRayCameraExport::filter()const\n";
	return "*.anm";
}
MPxFileTranslator::MFileKind CXRayCameraExport::identifyFile (
                                        const MFileObject& fileName,
                                        const char* buffer,
                                        short size) const
{
    const char * name = fileName.name().asChar();
    int   nameLength = xr_strlen(name);
    
	if ((nameLength > 4) && !stricmp(name+nameLength-4, ".anm"))
        return kCouldBeMyFileType;
    else
        return kNotMyFileType;
}

static IC void ParseMatrix	(MTransformationMatrix& mat, Fvector& t, Fvector& r, bool bRoot)
{
	MEulerRotation rot;
	MTransformationMatrix::RotationOrder ro=MTransformationMatrix::kZXY;
	mat.reorderRotation(ro);
	rot = mat.eulerRotation();

	r.set(-(float)rot.x, -(float)rot.y, (float)rot.z);

	MVector trans;
	trans = mat.translation(MSpace::kTransform);

	MDistance dst_x(trans.x);
	MDistance dst_y(trans.y);
	MDistance dst_z(trans.z);
	t.set((float)dst_x.asMeters(),(float)dst_y.asMeters(),-(float)dst_z.asMeters());

	if (bRoot){
		t.set((float)dst_x.asMeters(),(float)dst_y.asMeters(),-(float)dst_z.asMeters());
	}
}

bool correction_needed(float prev_ang, float curr_ang)
{
	float dist = _abs(curr_ang - prev_ang);
	if( _abs(dist-PI_MUL_2) < dist)
	{
//		Msg("needed [%f][%f]", prev_ang, curr_ang);
		return true;
	}else
		return false;
}

MStatus CXRayCameraExport::ExportCamera(const MFileObject& file)
{
	MDagPath            node; 
    MObject             component; 
    MSelectionList      list; 
    MFnDagNode          nodeFn; 
	MFnDagNode	C;
	MStatus		st ;
    
	MGlobal::getActiveSelectionList( list ); 
    for ( u32 index = 0; index < list.length(); ++index ) 
    { 
        list.getDagPath			( index, node, component ); 
        nodeFn.setObject		( node ); 
		st = C.setObject		(node);
		if(st!=MStatus::kSuccess)
		{
			Msg		("Selected object is not a camera");
			return	MStatus::kInvalidParameter;
		}

    } 

	Msg("exporting camera named [%s]",	C.name().asChar());

	MTime				tmTemp,tmTemp2;
	MTime				tmQuant;

	// Remember the frame the scene was at so we can restore it later.
	MTime storedFrame	= MAnimControl::currentTime();
	MTime startFrame	= MAnimControl::minTime();
	MTime endFrame		= MAnimControl::maxTime();

	tmTemp.setUnit		(MTime::uiUnit());
	tmTemp2.setUnit		(MTime::uiUnit());
	tmQuant.setUnit		(MTime::uiUnit());
	tmQuant				= 5.0; //10.0; //3 time in sec. temporary
	
	COMotion			M;
	int frms			= (int)(endFrame-startFrame).as(MTime::uiUnit());
	M.SetParam			(0, frms, 30);
	
	Fvector				P,R,Rprev;
	Rprev.set			(0,0,0);
	tmTemp				= startFrame;

//	MObject cam_parent	= C.parent(0);
//	MFnTransform		parentTransform(cam_parent);
	MStatus stat;	
	MDagPath			cam_path;
	stat				= C.getPath(cam_path);
	stat				= cam_path.extendToShape();

	MDistance			dist;
	bool b_not_first	= false;
	u32 count			= 0;
	float add_			= 0.0f;
	while(tmTemp <= endFrame)
	{
		MAnimControl::setCurrentTime		(tmTemp);
		MMatrix pM							= cam_path.inclusiveMatrix(&stat);
		MTransformationMatrix parentMatrix	(pM);

		ParseMatrix							(parentMatrix, P, R, true);
		
		R.y				+= add_;

		bool bpositive_y_prev	= Rprev.y>0.0f;
		bool bpositive_y_cur	= R.y>0.0f;
		if(b_not_first && correction_needed(Rprev.y, R.y))
		{
			if(bpositive_y_prev && !bpositive_y_cur) 
			{
				add_			= PI_MUL_2;
				R.y				+= add_;
				Msg("+2pi correction");
			}else
			if(!bpositive_y_prev && bpositive_y_cur) 
			{
				add_			= -PI_MUL_2;
				R.y				+= add_;
				Msg("-2pi correction");
			}else
			{
				float dist = (R.y - Rprev.y);
				if(dist>0)
				{
					add_			= -PI_MUL_2;
					R.y				+= add_;
				}else
				{
					add_			= PI_MUL_2;
					R.y				+= add_;
				}
			
			}
		}

		Rprev						= R;
		tmTemp2						= tmTemp-startFrame;
		float time_					= float(tmTemp2.as(MTime::uiUnit()))/30.0f;
		Msg							("%f - %f", time_, R.y);
		M.CreateKey					(time_, P, R);
		count++;
		if(tmTemp==endFrame)
			break;

		tmTemp						+= tmQuant;

		if(tmTemp>endFrame)
			tmTemp=endFrame;

		b_not_first					=true;
	};

	

	MString 			fn_save_to = file.fullName();
	fn_save_to			+= ".anm";

	Msg("file full name [%s]", fn_save_to);
	M.SaveMotion		(fn_save_to.asChar());

	MAnimControl::setCurrentTime( storedFrame );

	return MS::kSuccess;
}

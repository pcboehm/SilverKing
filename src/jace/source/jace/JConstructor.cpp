#include "jace/JConstructor.h"

#include "jace/Jace.h"

#include "jace/JArguments.h"
using jace::JArguments;

#include "jace/JSignature.h"
using jace::JSignature;

#include "jace/proxy/types/JVoid.h"
using jace::proxy::types::JVoid;

#include "jace/proxy/JValue.h"
using jace::proxy::JValue;

#include <vector>
using std::vector;

#include <string>
using std::string;

#include <iostream>
using std::cout;
using std::endl;

#include <list>
using std::list;

BEGIN_NAMESPACE(jace)

namespace
{
	// Transforms a JArguments to a vector of jvalue's
	vector<jvalue> toVector(const JArguments& arguments)
	{
		typedef list<const JValue*> ValueList;
		vector<jvalue> argsVector;
		ValueList argsList = arguments.asList();

		ValueList::iterator end = argsList.end();

		for (ValueList::iterator i = argsList.begin(); i != end; ++i)
			argsVector.push_back(static_cast<jvalue>(**i));

		return argsVector;
	}

} // namespace {


/**
 * Creates a new JConstructor for the given JClass.
 */
JConstructor::JConstructor(const JClass& javaClass):
  mClass(javaClass), mMethodID(0)
{}


/**
 * Invokes the constructor with the given JArguments.
 * Allocates a new local reference.
 *
 * @throws JNIException if an error occurs while trying to invoke the constructor.
 * @throws a matching C++ proxy, if a java exception is thrown by the constructor.
 */
jobject JConstructor::invoke(const JArguments& arguments)
{
  // Get the methodID for the constructor matching the given arguments.
//  cout << "JConstructor::invoke - Retrieving the methodID..." << endl;
  jmethodID methodID = getMethodID(mClass, arguments);
//  cout << "JConstructor::invoke - Retrieved the methodID." << endl;

  // Call the constructor
//  cout << "JConstructor::invoke - Attaching..." << endl;
  JNIEnv* env = attach();
//  cout << "JConstructor::invoke - Attached." << endl;

//  cout << "JConstructor::invoke - Creating the object..." << endl;
  jobject result;
	vector<jvalue> argArray = toVector(arguments);

	if (argArray.size() > 0)
		result = env->NewObjectA(mClass.getClass(), methodID, &argArray[0]);
	else
		result = env->NewObject(mClass.getClass(), methodID);
//  cout << "JConstructor::invoke - Created the object..." << endl;

  // Catch any java exception that occurred during the method call,
  // and throw it as a C++ exception.
//  cout << "JConstructor::invoke - Checking for exceptions..." << endl;
  catchAndThrow();
//  cout << "JConstructor::invoke - Found no exceptions." << endl;

  return result;
}


/**
 * Gets the method id matching the given arguments.
 */
jmethodID JConstructor::getMethodID(const JClass& jClass, const JArguments& arguments)
{
  // We cache the jmethodID locally, so if we've already found it,
  // we don't need to go looking for it again.
  if (mMethodID)
    return mMethodID;

  // If we don't already have the jmethodID, we need to determine
  // the signature of this method.

  // We construct this signature with a void return type,
  // because the return type for constructors is void.
  JSignature signature(JVoid::staticGetJavaJniClass());
  typedef list<const JValue*> ValueList;
  ValueList args = arguments.asList();

  ValueList::iterator i = args.begin();
  ValueList::iterator end = args.end();

  for (; i != end; ++i)
	{
    const JValue* value = *i;
    signature << value->getJavaJniClass();
  }

  string methodSignature = signature.toString();

  // Now that we have the signature for the method, we could look
  // in a global cache for the jmethodID corresponding to this method,
  // but for now, we'll always find it.
  JNIEnv* env = attach();

  mMethodID = env->GetMethodID(jClass.getClass(), "<init>", methodSignature.c_str());

  if (mMethodID == 0)
	{
		string msg = string("JConstructor::getMethodID(): ") +
                 "Unable to find constructor for " + jClass.getInternalName() + 
				 " with signature " + methodSignature;
		try
		{
			catchAndThrow();
		}
		catch (std::exception& e)
		{
			msg.append("\ncaused by:\n");
			msg.append(e.what());
		}
    throw JNIException(msg);
  }

  return mMethodID;
}


END_NAMESPACE(jace)

//===- SVFSymbols.h -- SVF symbols and variables------------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

/*
 * SVFSymbols.h
 *
 *  Created on: Nov 11, 2013
 *      Author: Yulei Sui
 */

#ifndef OBJECTANDSYMBOL_H_
#define OBJECTANDSYMBOL_H_

#include "MemoryModel/LocationSet.h"
#include "Util/SVFModule.h"

namespace SVF
{

class ObjTypeInfo;

/// Symbol symbol/variable types
enum SYMTYPE
{
    BlackHole,
    ConstantObj,
    BlkPtr,
    NullPtr,
    ValSymbol,
    ObjSymbol,
    RetSymbol,
    VarargSymbol
};


/*
* Variable symbols in SVF
*/
class SVFVar 
{

private:
    /// The unique value of this symbol/variable
    const Value *refVal;
    /// The unique id to represent this symbol
    SymID symId;
    /// The type of this symbol/variable
    u32_t kind;
public:

    /// Constructor
    SVFVar(SymID id, u32_t k, const Value *val): symId(id), refVal(val), kind(k){
    }

    /// Destructor
    virtual ~SVFVar()
    {
    }

    /// Get the reference value to this object
    inline const Value* getValue() const
    {
        return refVal;
    }

    /// Get the memory object id
    inline SymID getId() const
    {
        return symId;
    }

    /// ClassOf
    //@{
    static inline bool classof(const SVFVar*)
    {
        return true;
    }
    ///@}

    virtual const std::string toString() const = 0;
    
    /// Overloading operator << for printing
    //@{
    friend raw_ostream& operator<< (raw_ostream &o, const SVFVar &var)
    {
        o << var.toString();
        return o;
    }
    //@}

    inline u32_t getKind() const {
        return kind;
    }
};

/*
* Value symbols (top-level variables in LLVM-based languages)
*/
class ValSym : public SVFVar
{

public:
    ValSym(SymID id, const Value* val) : SVFVar(id, SYMTYPE::ValSymbol, val){
    }
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const ValSym *)
    {
        return true;
    }
    static inline bool classof(const SVFVar *var)
    {
        return var->getKind() == SYMTYPE::ValSymbol ||
               var->getKind() == SYMTYPE::BlkPtr ||
               var->getKind() == SYMTYPE::NullPtr;
    }
    //@}
    virtual const std::string toString() const;
};


/*!
 * Memory object symbols or MemObj (address-taken variables in LLVM-based languages)
 */
class ObjSym : public SVFVar
{

private:
    /// Type information of this object
    ObjTypeInfo* typeInfo;

public:

    /// Constructor
    ObjSym(SymID id, ObjTypeInfo* ti, const Value *val = nullptr);

    /// Destructor
    ~ObjSym()
    {
        destroy();
    }

    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const ObjSym *)
    {
        return true;
    }
    static inline bool classof(const SVFVar *var)
    {
        return var->getKind() == SYMTYPE::ObjSymbol ||
               var->getKind() == SYMTYPE::BlackHole ||
               var->getKind() == SYMTYPE::ConstantObj;
    }
    //@}
    virtual const std::string toString() const;

    /// Get obj type
    const Type* getType() const;

    /// Get max field offset limit
    Size_t getMaxFieldOffsetLimit() const;

    /// Return true if its field limit is 0
    bool isFieldInsensitive() const;

    /// Set the memory object to be field insensitive
    void setFieldInsensitive();

    /// Set the memory object to be field sensitive (up to max field limit)
    void setFieldSensitive();

    /// Whether it is a black hole object
    bool isBlackHoleObj() const;

    /// object attributes methods
    //@{
    bool isFunction() const;
    bool isGlobalObj() const;
    bool isStaticObj() const;
    bool isStack() const;
    bool isHeap() const;
    bool isStruct() const;
    bool isArray() const;
    bool isVarStruct() const;
    bool isVarArray() const;
    bool isConstStruct() const;
    bool isConstArray() const;
    bool isConstant() const;
    bool hasPtrObj() const;
    bool isNonPtrFieldObj(const LocationSet& ls) const;
    //@}

    /// Operator overloading
    inline bool operator==(const ObjSym &mem) const
    {
        return getValue() == mem.getValue();
    }

    /// Clean up memory
    void destroy();
};

typedef ObjSym MemObj;

/*
* BlockHole a unique sym used in points-to analysis to represent every possible objects
*/
class BlackHoleSym : public ObjSym
{

public:
    BlackHoleSym(SymID id, ObjTypeInfo* ti) : ObjSym(id, ti){

    }
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const BlackHoleSym *)
    {
        return true;
    }
    static inline bool classof(const ObjSym *var)
    {
        return var->getKind() == SYMTYPE::BlackHole;
    }
    static inline bool classof(const SVFVar *var)
    {
        return var->getKind() == SYMTYPE::BlackHole;
    }
    //@}
    virtual const std::string toString() const;
};

/*
* A unique sym to represent all constant objects if `Options::ModelConsts` is not turned on
*/
class ConstantObjSym : public ObjSym
{

public:
    ConstantObjSym(SymID id, ObjTypeInfo* ti) : ObjSym(id, ti){

    }
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const ConstantObjSym *)
    {
        return true;
    }
    static inline bool classof(const ObjSym *var)
    {
        return var->getKind() == SYMTYPE::ConstantObj;
    }
    static inline bool classof(const SVFVar *var)
    {
        return var->getKind() == SYMTYPE::ConstantObj;
    }
    //@}
    virtual const std::string toString() const;
};

/*
* A unique pointer sym, which points to BlackHoleSym
*/
class BlkPtrSym : public ValSym
{

public:
    BlkPtrSym(SymID id) : ValSym(id, nullptr){

    }
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const BlkPtrSym *)
    {
        return true;
    }
    static inline bool classof(const ValSym *var)
    {
        return var->getKind() == SYMTYPE::BlkPtr;
    }
    static inline bool classof(const SVFVar *var)
    {
        return var->getKind() == SYMTYPE::BlkPtr;
    }
    //@}
    virtual const std::string toString() const;
};


/*
* A unique pointer sym, which points to null
*/
class NullPtrSym : public ValSym
{

public:
    NullPtrSym(SymID id) : ValSym(id,nullptr){

    }
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const NullPtrSym *)
    {
        return true;
    }
    static inline bool classof(const ValSym *var)
    {
        return var->getKind() == SYMTYPE::NullPtr;
    }
    static inline bool classof(const SVFVar *var)
    {
        return var->getKind() == SYMTYPE::NullPtr;
    }
    //@}
    virtual const std::string toString() const;
};

/*
* SVF intrinsic symbols which are not explicitly presented in a program.
*/
class IntrinsicSym : public SVFVar
{

public:
    IntrinsicSym(SymID id, u32_t type, const Value* val) : SVFVar(id, type, val){
    }

    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const IntrinsicSym *)
    {
        return true;
    }
    static inline bool classof(const SVFVar *var)
    {
        return var->getKind() == SYMTYPE::RetSymbol ||
               var->getKind() == SYMTYPE::VarargSymbol;
    }
    //@}
    virtual const std::string toString() const = 0;
};


/*
* RetSym used to represent the unique return of a program function
*/
class RetSym : public IntrinsicSym
{

public:
    RetSym(SymID id, const Function* fun) : IntrinsicSym(id, SYMTYPE::RetSymbol, fun){
    }
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const RetSym *)
    {
        return true;
    }
    static inline bool classof(const IntrinsicSym *var)
    {
        return var->getKind() == SYMTYPE::RetSymbol;
    }
    static inline bool classof(const SVFVar *var)
    {
        return var->getKind() == SYMTYPE::RetSymbol;
    }
    //@}
    virtual const std::string toString() const;
};

/*
* VarargSym used to represent the varargs of a program function
*/
class VarargSym : public IntrinsicSym
{

public:
    VarargSym(SymID id, const Function* fun) : IntrinsicSym(id, SYMTYPE::VarargSymbol, fun){
    }
    /// Methods for support type inquiry through isa, cast, and dyn_cast:
    //@{
    static inline bool classof(const VarargSym *)
    {
        return true;
    }
    static inline bool classof(const IntrinsicSym *var)
    {
        return var->getKind() == SYMTYPE::VarargSymbol;
    }
    static inline bool classof(const SVFVar *var)
    {
        return var->getKind() == SYMTYPE::VarargSymbol;
    }
    //@}
    virtual const std::string toString() const;
};



/*!
 * Struct information
 */
class StInfo
{

private:
    /// flattened field indices of a struct
    std::vector<u32_t> fldIdxVec;
    /// flattened field offsets of of a struct
    std::vector<u32_t> foffset;
    /// Types of all fields of a struct
    Map<u32_t, const llvm::Type*> fldIdx2TypeMap;
    /// Types of all fields of a struct
    Map<u32_t, const llvm::Type*> offset2TypeMap;
    /// All field infos after flattening a struct
    std::vector<FieldInfo> finfo;

    /// Max field limit
    static u32_t maxFieldLimit;

public:
    /// Constructor
    StInfo()
    {
    }
    /// Destructor
    ~StInfo()
    {
    }

    static inline void setMaxFieldLimit(u32_t limit)
    {
        maxFieldLimit = limit;
    }

    static inline u32_t getMaxFieldLimit()
    {
        return maxFieldLimit;
    }

    /// Get method for fields of a struct
    //{@
    inline const llvm::Type* getFieldTypeWithFldIdx(u32_t fldIdx)
    {
        return fldIdx2TypeMap[fldIdx];
    }
    inline const llvm::Type* getFieldTypeWithByteOffset(u32_t offset)
    {
        return offset2TypeMap[offset];
    }
    inline std::vector<u32_t>& getFieldIdxVec()
    {
        return fldIdxVec;
    }
    inline std::vector<u32_t>& getFieldOffsetVec()
    {
        return foffset;
    }
    inline std::vector<FieldInfo>& getFlattenFieldInfoVec()
    {
        return finfo;
    }
    //@}

    /// Add field (index and offset) with its corresponding type
    inline void addFldWithType(u32_t fldIdx, u32_t offset, const llvm::Type* type)
    {
        fldIdxVec.push_back(fldIdx);
        foffset.push_back(offset);
        fldIdx2TypeMap[fldIdx] = type;
        offset2TypeMap[offset] = type;
    }
};

/*!
 * Type Info of an abstract memory object
 */
class ObjTypeInfo
{

public:
    typedef enum
    {
        FUNCTION_OBJ = 0x1,  // object is a function
        GLOBVAR_OBJ = 0x2,  // object is a global variable
        STATIC_OBJ = 0x4,  // object is a static variable allocated before main
        STACK_OBJ = 0x8,  // object is a stack variable
        HEAP_OBJ = 0x10,  // object is a heap variable
        VAR_STRUCT_OBJ = 0x20,  // object contains struct
        VAR_ARRAY_OBJ = 0x40,  // object contains array
        CONST_STRUCT_OBJ = 0x80,  // constant struct
        CONST_ARRAY_OBJ = 0x100,  // constant array
        CONST_OBJ = 0x200,  // constant object str e.g.
        HASPTR_OBJ = 0x400		// non pointer object including compound type have field that is a pointer type
    } MEMTYPE;

private:
    /// LLVM type
    const Type* type;
    /// Type flags
    Size_t flags;
    /// Max offset for flexible field sensitive analysis
    /// maximum number of field object can be created
    /// minimum number is 0 (field insensitive analysis)
    u32_t maxOffsetLimit;
public:

    /// Constructors
    ObjTypeInfo(const Value*, const Type* t, u32_t max) :
        type(t), flags(0), maxOffsetLimit(max)
    {
    }
    /// Constructor
    ObjTypeInfo(u32_t max, const Type* t) : type(t), flags(0), maxOffsetLimit(max)
    {

    }
    /// Destructor
    virtual ~ObjTypeInfo()
    {

    }

    /// Analyse types of heap and static objects
    void analyzeHeapObjType(const Type* type);

    /// Analyse types of heap and static objects
    void analyzeStaticObjType(const Type* type);

    /// Get LLVM type
    inline const Type* getType() const
    {
        return type;
    }

    /// Get max field offset limit
    inline u32_t getMaxFieldOffsetLimit()
    {
        return maxOffsetLimit;
    }

    /// Get max field offset limit
    inline void setMaxFieldOffsetLimit(u32_t limit)
    {
        maxOffsetLimit = limit;
    }

    /// Flag for this object type
    //@{
    inline void setFlag(MEMTYPE mask)
    {
        flags |= mask;
    }
    inline bool hasFlag(MEMTYPE mask)
    {
        return (flags & mask) == mask;
    }
    //@}

    /// Object attributes
    //@{
    inline bool isFunction()
    {
        return hasFlag(FUNCTION_OBJ);
    }
    inline bool isGlobalObj()
    {
        return hasFlag(GLOBVAR_OBJ);
    }
    inline bool isStaticObj()
    {
        return hasFlag(STATIC_OBJ);
    }
    inline bool isStack()
    {
        return hasFlag(STACK_OBJ);
    }
    inline bool isHeap()
    {
        return hasFlag(HEAP_OBJ);
    }
    //@}

    /// Object attributes (noted that an object can be a nested compound types)
    /// e.g. both isStruct and isArray can return true
    //@{
    inline bool isVarStruct()
    {
        return hasFlag(VAR_STRUCT_OBJ);
    }
    inline bool isConstStruct()
    {
        return hasFlag(CONST_STRUCT_OBJ);
    }
    inline bool isStruct()
    {
        return hasFlag(VAR_STRUCT_OBJ) || hasFlag(CONST_STRUCT_OBJ);
    }
    inline bool isVarArray()
    {
        return hasFlag(VAR_ARRAY_OBJ);
    }
    inline bool isConstArray()
    {
        return  hasFlag(CONST_ARRAY_OBJ);
    }
    inline bool isArray()
    {
        return hasFlag(VAR_ARRAY_OBJ) || hasFlag(CONST_ARRAY_OBJ);
    }
    inline bool isConstant()
    {
        return hasFlag(CONST_OBJ);
    }
    inline bool hasPtrObj()
    {
        return hasFlag(HASPTR_OBJ);
    }
    virtual bool isNonPtrFieldObj(const LocationSet& ls);
    //@}
};

} // End namespace SVF




#endif /* OBJECTANDSYMBOL_H_ */
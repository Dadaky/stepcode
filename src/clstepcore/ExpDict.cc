
/*
* NIST STEP Core Class Library
* clstepcore/ExpDict.cc
* April 1997
* K. C. Morris
* David Sauder

* Development of this software was funded by the United States Government,
* and is not subject to copyright.
*/

#include <sc_cf.h>

#include <memory.h>
#include <math.h>
#include <stdio.h>

#include <ExpDict.h>
#include <STEPaggregate.h>
#include "sc_memmgr.h"


Dictionary_instance__set::Dictionary_instance__set( int defaultSize ) {
    _bufsize = defaultSize;
    _buf = new Dictionary_instance_ptr[_bufsize];
    _count = 0;
}

Dictionary_instance__set::~Dictionary_instance__set() {
    delete[] _buf;
}

void Dictionary_instance__set::Check( int index ) {
    Dictionary_instance_ptr * newbuf;

    mtx.lock();
    if( index >= _bufsize ) {
        _bufsize = ( index + 1 ) * 2;
        newbuf = new Dictionary_instance_ptr[_bufsize];
        memmove( newbuf, _buf, _count * sizeof( Dictionary_instance_ptr ) );
        delete[] _buf;
        _buf = newbuf;
    }
    mtx.unlock();
}

void Dictionary_instance__set::Insert( Dictionary_instance_ptr v, int index ) {
    Dictionary_instance_ptr * spot;

    mtx.lock();
    index = ( index < 0 ) ? _count : index;
    if( index < _count ) {
        Check( _count + 1 );
        spot = &_buf[index];
        memmove( spot + 1, spot, ( _count - index )*sizeof( Dictionary_instance_ptr ) );

    } else {
        Check( index );
        spot = &_buf[index];
    }
    *spot = v;
    ++_count;
    mtx.unlock();
}

void Dictionary_instance__set::Append( Dictionary_instance_ptr v ) {
    Dictionary_instance_ptr * spot;

    mtx.lock();
    int index = _count;
    if( index < _count ) {
        Check( _count + 1 );
        spot = &_buf[index];
        memmove( spot + 1, spot, ( _count - index )*sizeof( Dictionary_instance_ptr ) );

    } else {
        Check( index );
        spot = &_buf[index];
    }
    *spot = v;
    ++_count;
    mtx.unlock();
}

void Dictionary_instance__set::Remove( int index ) {
    mtx.lock();
    if( 0 <= index && index < _count ) {
        --_count;
        Dictionary_instance_ptr * spot = &_buf[index];
        memmove( spot, spot + 1, ( _count - index )*sizeof( Dictionary_instance_ptr ) );
    }
    mtx.unlock();
}

int Dictionary_instance__set::Index( Dictionary_instance_ptr v ) {
    int index = -1;
    mtx.lock();
    for( int i = 0; i < _count; ++i ) {
        if( _buf[i] == v ) {
            index = i;
            break;
        }
    }
    mtx.unlock();
    return index;
}

Dictionary_instance_ptr & Dictionary_instance__set::operator[]( int index ) {
    mtx.lock();
    Check( index );
    _count = ( ( _count > index + 1 ) ? _count : ( index + 1 ) );
    Dictionary_instance_ptr & dip = _buf[index];
    mtx.unlock();
    return dip;
}

int Dictionary_instance__set::Count() {
    return _count;
}

void Dictionary_instance__set::Clear() {
    _count = 0;
}

///////////////////////////////////////////////////////////////////////////////

Explicit_item_id__set::Explicit_item_id__set( int defaultSize ) : Dictionary_instance__set( defaultSize ) {
}

Explicit_item_id__set::~Explicit_item_id__set() {
}

Explicit_item_id_ptr & Explicit_item_id__set::operator[]( int index ) {
    return ( Explicit_item_id_ptr & )Dictionary_instance__set::operator[]( index );
}

///////////////////////////////////////////////////////////////////////////////

Implicit_item_id__set::Implicit_item_id__set( int defaultSize ) : Dictionary_instance__set( defaultSize ) {
}


Implicit_item_id__set::~Implicit_item_id__set() {
}

Implicit_item_id_ptr & Implicit_item_id__set::operator[]( int index ) {
    return ( Implicit_item_id_ptr & )Dictionary_instance__set::operator[]( index );
}

///////////////////////////////////////////////////////////////////////////////

Interface_spec__set::Interface_spec__set( int defaultSize ) : Dictionary_instance__set( defaultSize ) {
}

Interface_spec__set::~Interface_spec__set() {
}

Interface_spec_ptr & Interface_spec__set::operator[]( int index ) {
    return ( Interface_spec_ptr & )Dictionary_instance__set::operator[]( index );
}

///////////////////////////////////////////////////////////////////////////////

Interface_spec::Interface_spec()
    : _explicit_items( new Explicit_item_id__set ),
      _implicit_items( 0 ), _all_objects( 0 ) {
}

/// not tested
Interface_spec::Interface_spec( Interface_spec & is ): Dictionary_instance() {
    _explicit_items = new Explicit_item_id__set;
    int count = is._explicit_items->Count();
    int i;
    for( i = 0; i < count; i++ ) {
        ( *_explicit_items )[i] =
            ( *( is._explicit_items ) )[i];
    }
    _current_schema_id = is._current_schema_id;
    _foreign_schema_id = is._foreign_schema_id;
    _all_objects = is._all_objects;
    _implicit_items = 0;
}

Interface_spec::Interface_spec( const char * cur_sch_id,
                                const char * foreign_sch_id, int all_objects )
    : _current_schema_id( cur_sch_id ), _explicit_items( new Explicit_item_id__set ),
      _implicit_items( 0 ), _foreign_schema_id( foreign_sch_id ),
      _all_objects( all_objects ) {
}

Interface_spec::~Interface_spec() {
    delete _explicit_items;
    delete _implicit_items;
}

//////////////////////////////////////////////////////////////////////////////
void Schema::AddFunction( const std::string & f ) {
    _function_list_mtx.lock();
    _function_list.push_back( f );
    _function_list_mtx.unlock();
}

void Schema::AddGlobal_rule( Global_rule_ptr gr ) {
    // locking here is required to prevent _global_rules from initializing twice
    _global_rules_mtx.lock();
    if( _global_rules == 0 ) {
        _global_rules = new Global_rule__set;
    }
    _global_rules->Append( gr );
    _global_rules_mtx.unlock();
}

/// hope I did this right (MP) - was "not implemented"
void Schema::global_rules_( Global_rule__set_var & grs ) {
    _global_rules_mtx.lock();
    if( _global_rules ) {
        if( _global_rules->Count() > 0 ) {
            std::cerr << "In " << __FILE__ << ", Schema::global_rules_(): overwriting non-empty global rule set!" << std::endl;
        }
        delete _global_rules;
    }
    _global_rules = grs;
    _global_rules_mtx.unlock();
}

void Schema::AddProcedure( const std::string & p ) {
    _procedure_list_mtx.lock();
    _procedure_list.push_back( p );
    _procedure_list_mtx.unlock();
}

/// the whole schema
void Schema::GenerateExpress( ostream & out ) const {
    std::string tmp;
    out << endl << "(* Generating: " << Name() << " *)" << endl;
    out << endl << "SCHEMA " << StrToLower( Name(), tmp ) << ";" << endl;
    GenerateUseRefExpress( out );
    // print TYPE definitions
    out << endl << "(* ////////////// TYPE Definitions *)" << endl;
    GenerateTypesExpress( out );

    // print Entity definitions
    out << endl << "(* ////////////// ENTITY Definitions *)" << endl;
    GenerateEntitiesExpress( out );

    int count, i;
    if( _global_rules != 0 ) {
        out << endl << "(* *************RULES************* *)" << endl;
        count = _global_rules->Count();
        for( i = 0; i <  count; i++ ) {
            out << endl << ( *_global_rules )[i]->rule_text_() << endl;
        }
    }
    if( !_function_list.empty() ) {
        out << "(* *************FUNCTIONS************* *)" << endl;
        count = _function_list.size();
        for( i = 0; i <  count; i++ ) {
            out << endl << _function_list[i] << endl;
        }
    }
    if( !_procedure_list.empty() ) {
        out << "(* *************PROCEDURES************* *)" << endl;
        count = _procedure_list.size();
        for( i = 0; i <  count; i++ ) {
            out << endl << _procedure_list[i] << endl;
        }
    }
    out << endl << "END_SCHEMA;" << endl;
}

/// USE, REFERENCE definitions
void Schema::GenerateUseRefExpress( ostream & out ) const {
    int i, k;
    int intf_count;
    int count;
    Interface_spec_ptr is;
    int first_time;
    std::string tmp;

    /////////////////////// print USE statements

    intf_count = _use_interface_list->Count();
    if( intf_count ) { // there is at least 1 USE interface to a foreign schema
        for( i = 0; i < intf_count; i++ ) { // print out each USE interface
            is = ( *_use_interface_list )[i]; // the 1st USE interface

            // count is # of USE items in interface
            count = is->explicit_items_()->Count();

            if( count > 0 ) {
                out << endl << "    USE FROM "
                    << StrToLower( is->foreign_schema_id_().c_str(), tmp ) << endl;
                out << "       (";

                first_time = 1;
                for( k = 0; k < count; k++ ) { // print out each USE item
                    if( first_time ) {
                        first_time = 0;
                    } else {
                        out << "," << endl << "\t";
                    }
                    if( !( ( *( is->explicit_items_() ) )[k]->original_id_().size() ) ) {
                        // not renamed
                        out << ( *( is->explicit_items_() ) )[k]->new_id_();
                    } else { // renamed
                        out << ( *( is->explicit_items_() ) )[k]->original_id_();
                        out << " AS " << ( *( is->explicit_items_() ) )[k]->new_id_();
                    }
                }
                out << ");" << endl;
            } else if( is->all_objects_() ) {
                out << endl << "    USE FROM "
                    << StrToLower( is->foreign_schema_id_().c_str(), tmp ) << ";"
                    << endl;
            }
        }
    }

    /////////////////////// print REFERENCE stmts

    intf_count = _ref_interface_list->Count();
    if( intf_count ) { //there is at least 1 REFERENCE interface to a foreign schema
        for( i = 0; i < intf_count; i++ ) { // print out each REFERENCE interface
            is = ( *_ref_interface_list )[i]; // the 1st REFERENCE interface

            // count is # of REFERENCE items in interface
            count = is->explicit_items_()->Count();


            if( count > 0 ) {
                out << endl << "    REFERENCE FROM "
                    << StrToLower( is->foreign_schema_id_().c_str(), tmp ) << endl;
                out << "       (";

                first_time = 1;
                for( k = 0; k < count; k++ ) { // print out each REFERENCE item
                    if( first_time ) {
                        first_time = 0;
                    } else {
                        out << "," << endl << "\t";
                    }
                    if( ( !( *( is->explicit_items_() ) )[k]->original_id_().size() ) ) {
                        // not renamed
                        out << ( *( is->explicit_items_() ) )[k]->new_id_();
                    } else { // renamed
                        out << ( *( is->explicit_items_() ) )[k]->original_id_();
                        out << " AS "
                            << ( *( is->explicit_items_() ) )[k]->new_id_();
                    }
                }
                out << ");" << endl;
            } else if( is->all_objects_() ) {
                out << endl << "    REFERENCE FROM "
                    << StrToLower( is->foreign_schema_id_().c_str(), tmp ) << ";"
                    << endl;
            }
        }
    }
}

/// TYPE definitions
void Schema::GenerateTypesExpress( ostream & out ) const {
    _typeList.mtxP->lock();
    TypeDescItr tdi( _typeList );
    tdi.ResetItr();
    std::string tmp;

    const TypeDescriptor * td = tdi.NextTypeDesc();
    while( td ) {
        out << endl << td->GenerateExpress( tmp );
        td = tdi.NextTypeDesc();
    }
    _typeList.mtxP->unlock();
}

/// Entity definitions
void Schema::GenerateEntitiesExpress( ostream & out ) const {
    _entList.mtxP->lock();
    EntityDescItr edi( _entList );
    edi.ResetItr();
    std::string tmp;

    const EntityDescriptor * ed = edi.NextEntityDesc();
    while( ed ) {
        out << endl << ed->GenerateExpress( tmp );
        ed = edi.NextEntityDesc();
    }
    _entList.mtxP->unlock();
}

///////////////////////////////////////////////////////////////////////////////

EnumAggregate * create_EnumAggregate() {
    return new EnumAggregate;
}

GenericAggregate * create_GenericAggregate() {
    return new GenericAggregate;
}

EntityAggregate * create_EntityAggregate() {
    return new EntityAggregate;
}

SelectAggregate * create_SelectAggregate() {
    return new SelectAggregate;
}

StringAggregate * create_StringAggregate() {
    return new StringAggregate;
}

BinaryAggregate * create_BinaryAggregate() {
    return new BinaryAggregate;
}

RealAggregate * create_RealAggregate() {
    return new RealAggregate;
}

IntAggregate * create_IntAggregate() {
    return new IntAggregate;
}

const EntityDescriptor * EntityDescItr::NextEntityDesc() {
    if( cur ) {
        const EntityDescriptor * ed = cur->EntityDesc();
        cur = ( EntityDescLinkNode * )( cur->NextNode() );
        return ed;
    }
    return 0;
}

const AttrDescriptor * AttrDescItr::NextAttrDesc() {
    if( cur ) {
        const AttrDescriptor * ad = cur->AttrDesc();
        cur = ( AttrDescLinkNode * )( cur->NextNode() );
        return ad;
    }
    return 0;
}

const Inverse_attribute * InverseAItr::NextInverse_attribute() {
    if( cur ) {
        const Inverse_attribute * ia = cur->Inverse_attr();
        cur = ( Inverse_attributeLinkNode * )( cur->NextNode() );
        return ia;
    }
    return 0;
}

const TypeDescriptor * TypeDescItr::NextTypeDesc() {
    if( cur ) {
        const TypeDescriptor * td = cur->TypeDesc();
        cur = ( TypeDescLinkNode * )( cur->NextNode() );
        return td;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// AttrDescriptor functions
///////////////////////////////////////////////////////////////////////////////

const char * AttrDescriptor::AttrExprDefStr( std::string & s ) const {
    std::string buf;

    s = Name();
    s.append( " : " );
    if( _optional.asInt() == LTrue ) {
        s.append( "OPTIONAL " );
    }
    if( DomainType() ) {
        s.append( DomainType()->AttrTypeName( buf ) );
    }
    return const_cast<char *>( s.c_str() );
}

PrimitiveType AttrDescriptor::BaseType() const {
    if( _domainType ) {
        return _domainType->BaseType();
    }
    return UNKNOWN_TYPE;
}

int AttrDescriptor::IsAggrType() const {
    return ReferentType()->IsAggrType();
}

PrimitiveType AttrDescriptor::AggrElemType() const {
    if( IsAggrType() ) {
        return ReferentType()->AggrElemType();
    }
    return UNKNOWN_TYPE;
}

const TypeDescriptor * AttrDescriptor::AggrElemTypeDescriptor() const {
    if( IsAggrType() ) {
        return ReferentType()->AggrElemTypeDescriptor();
    }
    return 0;
}

const TypeDescriptor * AttrDescriptor::NonRefTypeDescriptor() const {
    if( _domainType ) {
        return _domainType->NonRefTypeDescriptor();
    }
    return 0;
}

PrimitiveType
AttrDescriptor::NonRefType() const {
    if( _domainType ) {
        return _domainType->NonRefType();
    }
    return UNKNOWN_TYPE;
}

PrimitiveType
AttrDescriptor::Type() const {
    if( _domainType ) {
        return _domainType->Type();
    }
    return UNKNOWN_TYPE;
}

/**
 * right side of attr def
 * NOTE this returns a \'const char * \' instead of an std::string
 */
const char * AttrDescriptor::TypeName() const {
    std::string buf;

    if( _domainType ) {
        return _domainType->AttrTypeName( buf );
    } else {
        return "";
    }
}

/// an expanded right side of attr def
const char *
AttrDescriptor::ExpandedTypeName( std::string & s ) const {
    s.clear();
    if( Derived() == LTrue ) {
        s = "DERIVE  ";
    }
    if( _domainType ) {
        std::string tmp;
        return const_cast<char *>( ( s.append( _domainType->TypeString( tmp ) ).c_str() ) );
    } else {
        return 0;
    }
}

const char * AttrDescriptor::GenerateExpress( std::string & buf ) const {
    std::string sstr;
    buf = AttrExprDefStr( sstr );
    buf.append( ";\n" );
    return const_cast<char *>( buf.c_str() );
}

///////////////////////////////////////////////////////////////////////////////
// Derived_attribute functions
///////////////////////////////////////////////////////////////////////////////

const char * Derived_attribute::AttrExprDefStr( std::string & s ) const {
    std::string buf;

    s.clear();
    if( Name() && strchr( Name(), '.' ) ) {
        s = "SELF\\";
    }
    s.append( Name() );
    s.append( " : " );
    if( DomainType() ) {
        s.append( DomainType()->AttrTypeName( buf ) );
    }
    if( _initializer ) { // this is supposed to exist for a derived attribute.
        s.append( " \n\t\t:= " );
        s.append( _initializer );
    }
    return const_cast<char *>( s.c_str() );
}

///////////////////////////////////////////////////////////////////////////////
// Inverse_attribute functions
///////////////////////////////////////////////////////////////////////////////

const char * Inverse_attribute::AttrExprDefStr( std::string & s ) const {
    std::string buf;

    s = Name();
    s.append( " : " );
    if( _optional.asInt() == LTrue ) {
        s.append( "OPTIONAL " );
    }
    if( DomainType() ) {
        s.append( DomainType()->AttrTypeName( buf ) );
    }
    s.append( " FOR " );
    s.append( _inverted_attr_id );
    return const_cast<char *>( s.c_str() );
}

///////////////////////////////////////////////////////////////////////////////
// EnumDescriptor functions
///////////////////////////////////////////////////////////////////////////////

EnumTypeDescriptor::EnumTypeDescriptor( const char * nm, PrimitiveType ft,
                                        Schema * origSchema,
                                        const char * d, EnumCreator f )
    : TypeDescriptor( nm, ft, origSchema, d ), CreateNewEnum( f ) {
}

SDAI_Enum * EnumTypeDescriptor::CreateEnum() {
    if( CreateNewEnum ) {
        return CreateNewEnum();
    } else {
        return 0;
    }
}

const char * EnumTypeDescriptor::GenerateExpress( std::string & buf ) const {
    char tmp[BUFSIZ];
    buf = "TYPE ";
    buf.append( StrToLower( Name(), tmp ) );
    buf.append( " = ENUMERATION OF \n  (" );
    const char * desc = Description();
    const char * ptr = &( desc[16] );

    while( *ptr != '\0' ) {
        if( *ptr == ',' ) {
            buf.append( ",\n  " );
        } else if( isupper( *ptr ) ) {
            buf += ( char )tolower( *ptr );
        } else {
            buf += *ptr;
        }
        ptr++;
    }
    buf.append( ";\n" );
///////////////
    // count is # of WHERE rules
    if( _where_rules != 0 ) {
        _where_rules->mtx.lock();
        int all_comments = 1;
        int count = _where_rules->Count();
        for( int i = 0; i < count; i++ ) { // print out each UNIQUE rule
            if( !( *( _where_rules ) )[i]->_label.size() ) {
                all_comments = 0;
            }
        }

        if( all_comments ) {
            buf.append( "  (* WHERE *)\n" );
        } else {
            buf.append( "  WHERE\n" );
        }

        for( int i = 0; i < count; i++ ) { // print out each WHERE rule
            if( !( *( _where_rules ) )[i]->_comment.empty() ) {
                buf.append( "    " );
                buf.append( ( *( _where_rules ) )[i]->comment_() );
            }
            if( ( *( _where_rules ) )[i]->_label.size() ) {
                buf.append( "    " );
                buf.append( ( *( _where_rules ) )[i]->label_() );
            }
        }
        _where_rules->mtx.unlock();
    }

    buf.append( "END_TYPE;\n" );
    return const_cast<char *>( buf.c_str() );
}

///////////////////////////////////////////////////////////////////////////////
// EntityDescriptor functions
///////////////////////////////////////////////////////////////////////////////

EntityDescriptor::EntityDescriptor( )
    : _abstractEntity( LUnknown ), _extMapping( LUnknown ),
      _uniqueness_rules( ( Uniqueness_rule__set_var )0 ), NewSTEPentity( 0 ) {
}

EntityDescriptor::EntityDescriptor( const char * name, // i.e. char *
                                    Schema * origSchema,
                                    Logical abstractEntity, // F U or T
                                    Logical extMapping,
                                    Creator f
                                  )
    : TypeDescriptor( name, ENTITY_TYPE, origSchema, name ),
      _abstractEntity( abstractEntity ), _extMapping( extMapping ),
      _uniqueness_rules( ( Uniqueness_rule__set_var )0 ), NewSTEPentity( f ) {
}

EntityDescriptor::~EntityDescriptor() {
    delete _uniqueness_rules;
}

const char * EntityDescriptor::GenerateExpress( std::string & buf ) const {
    std::string sstr;
    int count;
    int i;
    int all_comments = 1;

    buf = "ENTITY ";
    buf.append( StrToLower( Name(), sstr ) );

    if( strlen( _supertype_stmt.c_str() ) > 0 ) {
        buf.append( "\n  " );
    }
    buf.append( _supertype_stmt );

    const EntityDescriptor * ed = 0;

    _supertypes.mtxP->lock();
    EntityDescItr edi_super( _supertypes );
    edi_super.ResetItr();
    ed = edi_super.NextEntityDesc();
    int supertypes = 0;
    if( ed ) {
        buf.append( "\n  SUBTYPE OF (" );
        buf.append( StrToLower( ed->Name(), sstr ) );
        supertypes = 1;
    }
    ed = edi_super.NextEntityDesc();
    while( ed ) {
        buf.append( ",\n\t\t" );
        buf.append( StrToLower( ed->Name(), sstr ) );
        ed = edi_super.NextEntityDesc();
    }
    if( supertypes ) {
        buf.append( ")" );
    }
    _supertypes.mtxP->unlock();

    buf.append( ";\n" );

    _explicitAttr.mtxP->lock();
    AttrDescItr adi( _explicitAttr );

    adi.ResetItr();
    const AttrDescriptor * ad = adi.NextAttrDesc();

    while( ad ) {
        if( ad->AttrType() == AttrType_Explicit ) {
            buf.append( "    " );
            buf.append( ad->GenerateExpress( sstr ) );
        }
        ad = adi.NextAttrDesc();
    }

    adi.ResetItr();
    ad = adi.NextAttrDesc();

    count = 1;
    while( ad ) {
        if( ad->AttrType() == AttrType_Deriving ) {
            if( count == 1 ) {
                buf.append( "  DERIVE\n" );
            }
            buf.append( "    " );
            buf.append( ad->GenerateExpress( sstr ) );
            count++;
        }
        ad = adi.NextAttrDesc();
    }
    _explicitAttr.mtxP->unlock();
/////////

    _inverseAttr.mtxP->lock();
    InverseAItr iai( _inverseAttr );

    iai.ResetItr();
    const Inverse_attribute * ia = iai.NextInverse_attribute();

    if( ia ) {
        buf.append( "  INVERSE\n" );
    }

    while( ia ) {
        buf.append( "    " );
        buf.append( ia->GenerateExpress( sstr ) );
        ia = iai.NextInverse_attribute();
    }
    _inverseAttr.mtxP->unlock();
///////////////
    // count is # of UNIQUE rules
    if( _uniqueness_rules != 0 ) {
        _uniqueness_rules->mtx.lock();
        count = _uniqueness_rules->Count();
        for( i = 0; i < count; i++ ) { // print out each UNIQUE rule
            if( !( *( _uniqueness_rules ) )[i]->_label.size() ) {
                all_comments = 0;
            }
        }

        if( all_comments ) {
            buf.append( "  (* UNIQUE *)\n" );
        } else {
            buf.append( "  UNIQUE\n" );
        }
        for( i = 0; i < count; i++ ) { // print out each UNIQUE rule
            if( !( *( _uniqueness_rules ) )[i]->_comment.empty() ) {
                buf.append( "    " );
                buf.append( ( *( _uniqueness_rules ) )[i]->comment_() );
                buf.append( "\n" );
            }
            if( ( *( _uniqueness_rules ) )[i]->_label.size() ) {
                buf.append( "    " );
                buf.append( ( *( _uniqueness_rules ) )[i]->label_() );
                buf.append( "\n" );
            }
        }
        _uniqueness_rules->mtx.unlock();
    }

///////////////
    // count is # of WHERE rules
    if( _where_rules != 0 ) {
        _where_rules->mtx.lock();
        all_comments = 1;
        count = _where_rules->Count();
        for( i = 0; i < count; i++ ) { // print out each UNIQUE rule
            if( !( *( _where_rules ) )[i]->_label.size() ) {
                all_comments = 0;
            }
        }

        if( !all_comments ) {
            buf.append( "  WHERE\n" );
        } else {
            buf.append( "  (* WHERE *)\n" );
        }
        for( i = 0; i < count; i++ ) { // print out each WHERE rule
            if( !( *( _where_rules ) )[i]->_comment.empty() ) {
                buf.append( "    " );
                buf.append( ( *( _where_rules ) )[i]->comment_() );
                buf.append( "\n" );
            }
            if( ( *( _where_rules ) )[i]->_label.size() ) {
                buf.append( "    " );
                buf.append( ( *( _where_rules ) )[i]->label_() );
                buf.append( "\n" );
            }
        }
        _where_rules->mtx.unlock();
    }

    buf.append( "END_ENTITY;\n" );

    return const_cast<char *>( buf.c_str() );
}

const char * EntityDescriptor::QualifiedName( std::string & s ) const {
    s.clear();
    _supertypes.mtxP->lock();
    EntityDescItr edi( _supertypes );

    int count = 1;
    const EntityDescriptor * ed = edi.NextEntityDesc();
    while( ed ) {
        if( count > 1 ) {
            s.append( "&" );
        }
        s.append( ed->Name() );
        count++;
        ed = edi.NextEntityDesc();
    }
    _supertypes.mtxP->unlock();

    if( count > 1 ) {
        s.append( "&" );
    }
    s.append( Name() );
    return const_cast<char *>( s.c_str() );
}

const TypeDescriptor * EntityDescriptor::IsA( const TypeDescriptor * td ) const {
    if( td -> NonRefType() == ENTITY_TYPE ) {
        return IsA( ( EntityDescriptor * ) td );
    } else {
        return 0;
    }
}

const EntityDescriptor * EntityDescriptor::IsA( const EntityDescriptor * other )  const {
    const EntityDescriptor * found = 0;
    _supertypes.mtxP->lock();
    const EntityDescLinkNode * link = ( const EntityDescLinkNode * )( _supertypes.GetHead() );

    if( this == other ) {
        found = other;
    } else {
        while( link && ! found )  {
            found = link -> EntityDesc() -> IsA( other );
            link = ( EntityDescLinkNode * ) link -> NextNode();
        }
    }
    _supertypes.mtxP->unlock();
    return found;
}

Type_or_rule::Type_or_rule() {
    std::cerr << "WARNING - Type_or_rule class doesn't seem to be complete - it has no members!" << std::endl;
}

Type_or_rule::Type_or_rule( const Type_or_rule & tor ): Dictionary_instance() {
    (void) tor; //TODO once this class has some members, we'll actually have something to copy
}

Type_or_rule::~Type_or_rule() {
}

Where_rule::Where_rule() {
    _type_or_rule = 0;
}

Where_rule::Where_rule( const Where_rule & wr ): Dictionary_instance() {
    _label = wr._label;
    _type_or_rule = wr._type_or_rule;
}

Where_rule::~Where_rule() {
}

///////////////////////////////////////////////////////////////////////////////

Where_rule__list::Where_rule__list( int defaultSize ) : Dictionary_instance__set( defaultSize ) {
}

Where_rule__list::~Where_rule__list() {
    Clear();
}

Where_rule_ptr & Where_rule__list::operator[]( int index ) {
    return ( Where_rule_ptr & )Dictionary_instance__set::operator[]( index );
}

void Where_rule__list::Clear() {
    mtx.lock();
    for( int i = 0; i < _count ; i ++ ) {
        delete ( Where_rule_ptr )_buf[i];
    }
    _count = 0;
    mtx.unlock();
}

///////////////////////////////////////////////////////////////////////////////

Uniqueness_rule::Uniqueness_rule()
    : _parent_entity( 0 ) {
}

Uniqueness_rule::Uniqueness_rule( const Uniqueness_rule & ur ): Dictionary_instance() {
    _label = ur._label;
    _parent_entity = ur._parent_entity;
}

Uniqueness_rule::~Uniqueness_rule() {
    // don't delete _parent_entity
}

///////////////////////////////////////////////////////////////////////////////

Uniqueness_rule__set::Uniqueness_rule__set( int defaultSize ) : Dictionary_instance__set( defaultSize ) {
}

Uniqueness_rule__set::~Uniqueness_rule__set() {
    Clear();
}

Uniqueness_rule_ptr & Uniqueness_rule__set::operator[]( int index ) {
    return ( Uniqueness_rule_ptr & )Dictionary_instance__set::operator[]( index );
}

void Uniqueness_rule__set::Clear() {
    mtx.lock();
    for( int i = 0; i < _count; i ++ ) {
        delete ( Uniqueness_rule_ptr )_buf[i];
    }
    _count = 0;
    mtx.unlock();
}

///////////////////////////////////////////////////////////////////////////////

Global_rule::Global_rule()
    : _entities( 0 ), _where_rules( 0 ), _parent_schema( 0 ) {
}

Global_rule::Global_rule( const char * n, Schema_ptr parent_sch, const std::string & rt )
    : _name( n ), _entities( 0 ), _where_rules( 0 ), _parent_schema( parent_sch ),
      _rule_text( rt ) {
}

/// not fully implemented
Global_rule::Global_rule( Global_rule & gr ): Dictionary_instance() {
    _name = gr._name;
    _parent_schema = gr._parent_schema;
}

Global_rule::~Global_rule() {
    delete _entities;
    delete _where_rules;
}

void Global_rule::entities_( const Entity__set_var & e ) {
    // required to prevent potential double deletion
    mtx.lock();
    if( _entities ) {
        if( _entities->EntryCount() > 0 ) {
            std::cerr << "In " << __FILE__ << ", Global_rule::entities_(): overwriting non-empty entity set!" << std::endl;
        }
        delete _entities;
    }
    _entities = e;
    mtx.unlock();
}

void Global_rule::where_rules_( const Where_rule__list_var & wrl ) {
    mtx.lock();
    if( _where_rules ) {
        if( _where_rules->Count() > 0 ) {
            std::cerr << "In " << __FILE__ << ", Global_rule::where_rules_(): overwriting non-empty rule set!" << std::endl;
        }
        delete _where_rules;
    }
    _where_rules = wrl;
    mtx.unlock();
}

///////////////////////////////////////////////////////////////////////////////

Global_rule__set::Global_rule__set( int defaultSize ) : Dictionary_instance__set( defaultSize ) {
}

Global_rule__set::~Global_rule__set() {
    Clear();
}

Global_rule_ptr & Global_rule__set::operator[]( int index ) {
    return ( Global_rule_ptr & )Dictionary_instance__set::operator[]( index );
}

void Global_rule__set::Clear() {
    mtx.lock();
    for( int i = 0; i < _count; i ++ ) {
        delete ( Global_rule_ptr )_buf[i];
    }
    _count = 0;
    mtx.unlock();
}


///////////////////////////////////////////////////////////////////////////////
// TypeDescriptor functions
///////////////////////////////////////////////////////////////////////////////

const char * TypeDescriptor::AttrTypeName( std::string & buf, const char * schnm ) const {
    std::string sstr;
    buf = Name( schnm ) ? StrToLower( Name( schnm ), sstr ) : _description;
    return const_cast<char *>( buf.c_str() );
}

const char * TypeDescriptor::GenerateExpress( std::string & buf ) const {
    char tmp[BUFSIZ];
    buf = "TYPE ";
    buf.append( StrToLower( Name(), tmp ) );
    buf.append( " = " );
    const char * desc = Description();
    const char * ptr = desc;

    while( *ptr != '\0' ) {
        if( *ptr == ',' ) {
            buf.append( ",\n  " );
        } else if( *ptr == '(' ) {
            buf.append( "\n  (" );
        } else if( isupper( *ptr ) ) {
            buf += ( char )tolower( *ptr );
        } else {
            buf += *ptr;
        }
        ptr++;
    }
    buf.append( ";\n" );
///////////////
    // count is # of WHERE rules
    if( _where_rules != 0 ) {
        _where_rules->mtx.lock();
        int all_comments = 1;
        int count = _where_rules->Count();
        for( int i = 0; i < count; i++ ) { // print out each UNIQUE rule
            if( !( *( _where_rules ) )[i]->_label.size() ) {
                all_comments = 0;
            }
        }

        if( all_comments ) {
            buf.append( "  (* WHERE *)\n" );
        } else {
            buf.append( "    WHERE\n" );
        }

        for( int i = 0; i < count; i++ ) { // print out each WHERE rule
            if( !( *( _where_rules ) )[i]->_comment.empty() ) {
                buf.append( "    " );
                buf.append( ( *( _where_rules ) )[i]->comment_() );
            }
            if( ( *( _where_rules ) )[i]->_label.size() ) {
                buf.append( "      " );
                buf.append( ( *( _where_rules ) )[i]->label_() );
            }
        }
        _where_rules->mtx.unlock();
    }

    buf.append( "END_TYPE;\n" );
    return const_cast<char *>( buf.c_str() );
}

/**
 * This is a fully expanded description of the type.
 * This returns a string like the _description member variable
 * except it is more thorough of a description where possible
 * e.g. if the description contains a TYPE name it will also
 * be explained.
 */
const char * TypeDescriptor::TypeString( std::string & s ) const {
    switch( Type() ) {
        case REFERENCE_TYPE:
            if( Name() ) {
                s.append( "TYPE " );
                s.append( Name() );
                s.append( " = " );
            }
            if( Description() ) {
                s.append( Description() );
            }
            if( ReferentType() ) {
                s.append( " -- " );
                std::string tmp;
                s.append( ReferentType()->TypeString( tmp ) );
            }
            return const_cast<char *>( s.c_str() );

        case INTEGER_TYPE:
            s.clear();
            if( _referentType != 0 ) {
                s = "TYPE ";
                s.append( Name() );
                s.append( " = " );
            }
            s.append( "Integer" );
            break;

        case STRING_TYPE:
            s.clear();
            if( _referentType != 0 ) {
                s = "TYPE ";
                s.append( Name() );
                s.append( " = " );
            }
            s.append( "String" );
            break;

        case REAL_TYPE:
            s.clear();
            if( _referentType != 0 ) {
                s = "TYPE ";
                s.append( Name() );
                s.append( " = " );
            }
            s.append( "Real" );
            break;

        case ENUM_TYPE:
            s = "Enumeration: ";
            if( Name() ) {
                s.append( "TYPE " );
                s.append( Name() );
                s.append( " = " );
            }
            if( Description() ) {
                s.append( Description() );
            }
            break;

        case BOOLEAN_TYPE:
            s.clear();
            if( _referentType != 0 ) {
                s = "TYPE ";
                s.append( Name() );
                s.append( " = " );
            }
            s.append( "Boolean: F, T" );
            break;
        case LOGICAL_TYPE:
            s.clear();
            if( _referentType != 0 ) {
                s = "TYPE ";
                s.append( Name() );
                s.append( " = " );
            }
            s.append( "Logical: F, T, U" );
            break;
        case NUMBER_TYPE:
            s.clear();
            if( _referentType != 0 ) {
                s = "TYPE ";
                s.append( Name() );
                s.append( " = " );
            }
            s.append( "Number" );
            break;
        case BINARY_TYPE:
            s.clear();
            if( _referentType != 0 ) {
                s = "TYPE ";
                s.append( Name() );
                s.append( " = " );
            }
            s.append( "Binary" );
            break;
        case ENTITY_TYPE:
            s = "Entity: ";
            if( Name() ) {
                s.append( Name() );
            }
            break;
        case AGGREGATE_TYPE:
        case ARRAY_TYPE:      // DAS
        case BAG_TYPE:        // DAS
        case SET_TYPE:        // DAS
        case LIST_TYPE:       // DAS
            s = Description();
            if( ReferentType() ) {
                s.append( " -- " );
                std::string tmp;
                s.append( ReferentType()->TypeString( tmp ) );
            }
            break;
        case SELECT_TYPE:
            s.append( Description() );
            break;
        case GENERIC_TYPE:
        case UNKNOWN_TYPE:
            s = "Unknown";
            break;
    } // end switch
    return const_cast<char *>( s.c_str() );

}

const TypeDescriptor * TypeDescriptor::IsA( const TypeDescriptor * other )  const {
    if( this == other ) {
        return other;
    }
    return 0;
}

const TypeDescriptor * TypeDescriptor::IsA( const char * other ) const  {
    if( !Name() ) {
        return 0;
    }
    if( !StrCmpIns( _name, other ) ) {   // this is the type
        return this;
    }
    return ( ReferentType() ? ReferentType() -> IsA( other ) : 0 );
}

/**
 * the first PrimitiveType that is not REFERENCE_TYPE (the first
 * TypeDescriptor *_referentType that does not have REFERENCE_TYPE
 * for it's fundamentalType variable).  This would return the same
 * as BaseType() for fundamental types.  An aggregate type
 * would return AGGREGATE_TYPE then you could find out the type of
 * an element by calling AggrElemType().  Select types
 * would work the same?
 */
PrimitiveType TypeDescriptor::NonRefType() const {
    const TypeDescriptor * td = NonRefTypeDescriptor();
    if( td ) {
        return td->FundamentalType();
    }
    return UNKNOWN_TYPE;
}


const TypeDescriptor * TypeDescriptor::NonRefTypeDescriptor() const {
    const TypeDescriptor * td = this;

    while( td->ReferentType() ) {
        if( td->Type() != REFERENCE_TYPE ) {
            return td;
        }
        td = td->ReferentType();
    }

    return td;
}

/// This returns the PrimitiveType of the first non-aggregate element of an aggregate
int TypeDescriptor::IsAggrType() const {
    switch( NonRefType() ) {
        case AGGREGATE_TYPE:
        case ARRAY_TYPE:      // DAS
        case BAG_TYPE:        // DAS
        case SET_TYPE:        // DAS
        case LIST_TYPE:       // DAS
            return 1;

        default:
            return 0;
    }
}

PrimitiveType TypeDescriptor::AggrElemType() const {
    const TypeDescriptor * aggrElemTD = AggrElemTypeDescriptor();
    if( aggrElemTD ) {
        return aggrElemTD->Type();
    }
    return UNKNOWN_TYPE;
}

const TypeDescriptor * TypeDescriptor::AggrElemTypeDescriptor() const {
    const TypeDescriptor * aggrTD = NonRefTypeDescriptor();
    const TypeDescriptor * aggrElemTD = aggrTD->ReferentType();
    if( aggrElemTD ) {
        aggrElemTD = aggrElemTD->NonRefTypeDescriptor();
    }
    return aggrElemTD;
}

/**
 * This is the underlying type of this type. For instance:
 * TYPE count = INTEGER;
 * TYPE ref_count = count;
 * TYPE count_set = SET OF ref_count;
 *  each of the above will generate a TypeDescriptor and for
 *  each one, PrimitiveType BaseType() will return INTEGER_TYPE
 *  TypeDescriptor *BaseTypeDescriptor() returns the TypeDescriptor
 *  for Integer
 */
PrimitiveType TypeDescriptor::BaseType() const {
    const TypeDescriptor * td = BaseTypeDescriptor();
    if( td ) {
        return td->FundamentalType();
    } else {
        return ENTITY_TYPE;
    }
}

const TypeDescriptor * TypeDescriptor::BaseTypeDescriptor() const {
    const TypeDescriptor * td = this;

    while( td -> ReferentType() ) {
        td = td->ReferentType();
    }
    return td;
}

/** FIXME
 * #ifdef NOT_YET
    ///////////////////////////////////////////////////////////////////////////////
    // EnumerationTypeDescriptor functions
    ///////////////////////////////////////////////////////////////////////////////
    EnumerationTypeDescriptor::EnumerationTypeDescriptor( ) {
        _elements = new StringAggregate;
    }
 * #endif
 */

///////////////////////////////////////////////////////////////////////////////
// SelectTypeDescriptor functions
///////////////////////////////////////////////////////////////////////////////

SDAI_Select * SelectTypeDescriptor::CreateSelect() {
    if( CreateNewSelect ) {
        return CreateNewSelect();
    } else {
        return 0;
    }
}

const TypeDescriptor * SelectTypeDescriptor::IsA( const TypeDescriptor * other ) const {
    return TypeDescriptor::IsA( other );
}

/**
 * returns the td among the choices of tds describing elements of this select
 * type but only at this unexpanded level. The td ultimately describing the
 * type may be an element of a td for a select that is returned.
 */
const TypeDescriptor * SelectTypeDescriptor::CanBe( const TypeDescriptor * other ) const {
    if( this == other ) {
        return other;
    }

    const TypeDescriptor * rettd = 0;
    const TypeDescriptorList & tl = GetElements();
    tl.mtxP->lock();
    TypeDescItr elements( tl ) ;
    const TypeDescriptor * td = elements.NextTypeDesc();
    while( td )  {
        if( td -> CanBe( other ) ) {
            rettd = td;
            break;
        }
        td = elements.NextTypeDesc();
    }
    tl.mtxP->unlock();
    return rettd;
}

/**
 * returns the td among the choices of tds describing elements of this select
 * type but only at this unexpanded level. The td ultimately describing the
 * type may be an element of a td for a select that is returned.
 */
const TypeDescriptor * SelectTypeDescriptor::CanBe( const char * other ) const {
    // see if other is the select
    if( !StrCmpIns( _name, other ) ) {
        return this;
    }

    const TypeDescriptor * td = 0;
    const TypeDescriptor * rettd = 0;
    const TypeDescriptorList & tl = GetElements();
    tl.mtxP->lock();

    TypeDescItr elements( tl ) ;
    // see if other is one of the elements
    while( ( td = elements.NextTypeDesc() ) ) {
        if( td -> CanBe( other ) ) {
            rettd = td;
            break;
        }
    }
    tl.mtxP->unlock();
    return rettd;
}

/**
 * A modified CanBe, used to determine if "other", a string we have just read,
 * is a possible type-choice of this.  (I.e., our select "CanBeSet" to this
 * choice.)  This deals with the following issue, based on the Tech Corrigendum
 * to Part 21:  Say our select ("selP") has an item which is itself a select
 * ("selQ").  Say it has another select item which is a redefinition of another
 * select ("TYPE selR = selS;").  According to the T.C., if selP is set to one
 * of the members of selQ, "selQ(...)" may not appear in the instantiation.
 * If, however, selP is set to a member of selR, "selR(...)" must appear first.
 * The code below checks if "other" = one of our possible choices.  If one of
 * our choices is a select like selQ, we recurse to see if other matches a
 * member of selQ (and don't look for "selQ").  If we have a choice like selR,
 * we check if other = "selR", but do not look at selR's members.  This func-
 * tion also takes into account schNm, the name of the current schema.  If
 * schNm does not = the schema in which this type was declared, it's possible
 * that it should be referred to with a different name.  This would be the case
 * if schNm = a schema which USEs or REFERENCEs this and renames it (e.g., "USE
 * from XX (A as B)").
 */
const TypeDescriptor * SelectTypeDescriptor::CanBeSet( const char * other, const char * schNm ) const {
    const TypeDescriptorList & tl = GetElements();
    tl.mtxP->lock();
    TypeDescItr elements( tl ) ;
    const TypeDescriptor * td = elements.NextTypeDesc();
    const TypeDescriptor * rettd = 0;

    while( td ) {
        if( td->Type() == REFERENCE_TYPE && td->NonRefType() == sdaiSELECT ) {
            // Just look at this level, don't look at my items (see intro).
            if( td->CurrName( other, schNm ) ) {
                rettd = td;
                break;
            }
        } else if( td->CanBeSet( other, schNm ) ) {
            rettd = td;
            break;
        }
        td = elements.NextTypeDesc();
    }
    tl.mtxP->unlock();
    return rettd;
}

///////////////////////////////////////////////////////////////////////////////
// AggrTypeDescriptor functions
///////////////////////////////////////////////////////////////////////////////

STEPaggregate * AggrTypeDescriptor::CreateAggregate() {
    if( CreateNewAggr ) {
        return CreateNewAggr();
    } else {
        return 0;
    }
}

void AggrTypeDescriptor::AssignAggrCreator( AggregateCreator f ) {
    CreateNewAggr = f;
}

AggrTypeDescriptor::AggrTypeDescriptor( ) :
    _uniqueElements( "UNKNOWN_TYPE" ) {
    _bound1 = -1;
    _bound2 = -1;
    _aggrDomainType = 0;
}

AggrTypeDescriptor::AggrTypeDescriptor( SDAI_Integer  b1,
                                        SDAI_Integer  b2,
                                        Logical uniqElem,
                                        TypeDescriptor * aggrDomType )
    : _bound1( b1 ), _bound2( b2 ), _uniqueElements( uniqElem ) {
    _aggrDomainType = aggrDomType;
}

AggrTypeDescriptor::~AggrTypeDescriptor() {
}

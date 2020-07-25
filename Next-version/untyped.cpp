/*           untyped C++ (Version 0.1 - 2012/07)
    <https://github.com/peychart/untyped-cpp>

    Copyright (C) 2017  -  peychart

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this program.
    If not, see <http://www.gnu.org/licenses/>.

    Details of this licence are available online under:
                        http://www.gnu.org/licenses/gpl-3.0.html
*/
#include "untyped.h"

namespace noType
{ unsigned short untyped::json = 0, untyped::tabSize = 0;

  untyped::untyped()                          : _type( 0) {
  }

  untyped::untyped( bool const v )            : _type( 1) {
    _set( sizeof(bool),     reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( char const v )            : _type( 2) {
    _set( sizeof(char),     reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( wchar_t const v )         : _type( 3) {
    _set( sizeof(wchar_t),  reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( int8_t const v )          : _type( 4) {
    _set( sizeof(int8_t),   reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( uint8_t const v )         : _type( 5) {
    _set( sizeof(uint8_t),  reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( int16_t const v )         : _type( 6) {
    _set( sizeof(int16_t),  reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( uint16_t const v )        : _type( 7) {
    _set( sizeof(uint16_t), reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( int32_t const v )         : _type( 8) {
    _set( sizeof(int32_t),  reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( uint32_t const v )        : _type( 9) {
    _set( sizeof(uint32_t), reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( int64_t const v )         : _type(10) {
    _set( sizeof(int64_t),  reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( uint64_t const v )        : _type(11) {
    _set( sizeof(uint64_t), reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( float const v )           : _type(12) {
    _set( sizeof(float),    reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( double const v )          : _type(13) {
    _set( sizeof(double),   reinterpret_cast<char const *>( &v ) );
  }

  untyped::untyped( char const *v )           : _type(15) {
    _set( stringSize( v ),  reinterpret_cast<char const *>(  v ) );
  }

  untyped::untyped( std::string const &v )    : _type(15) {
    _set( v.size(), v.data() );
  }

  untyped::untyped( size_t s, void const *v ) : _type(15) {
    _set( s, reinterpret_cast<char const *>( v ) );
  }

  untyped::untyped( untyped const &v )                    {
    assign(v);
  }

  untyped::untyped( std::istream &in )                    {
    deserialize( in );
  }

  untyped& untyped::assign( untyped const &v ) {
    clearVector(); _set( vectorType(v) );
    clearMap();    _set( mapType(v) );
    _set( v.size(), v.data() ); _type = v._type;
    return *this;
  }

  bool untyped::operator==( untyped const &that ) const {
    switch( _type ) {
      case  1: return( value<bool    >() == that.value<bool    >() );
      case  2: return( value<char    >() == that.value<char    >() );
      case  3: return( value<wchar_t >() == that.value<wchar_t >() );
      case  4: return( value<int8_t  >() == that.value<int8_t  >() );
      case  5: return( value<uint8_t >() == that.value<uint8_t >() );
      case  6: return( value<int16_t >() == that.value<int16_t >() );
      case  7: return( value<uint16_t>() == that.value<uint16_t>() );
      case  8: return( value<int32_t >() == that.value<int32_t >() );
      case  9: return( value<uint32_t>() == that.value<uint32_t>() );
      case 10: return( value<int64_t >() == that.value<int64_t >() );
      case 11: return( value<uint64_t>() == that.value<uint64_t>() );
      case 12: return( value<float   >() == that.value<float   >() );
      case 13: return( value<double  >() == that.value<double  >() );
      case 15: return( !this->compare(that) );
      default: return( _type==that._type );
    } return true;
  }

  bool untyped::operator<( untyped const &that ) const {
    switch( _type ) {
      case  1: return( value<bool    >() < that.value<bool    >() );
      case  2: return( value<char    >() < that.value<char    >() );
      case  3: return( value<wchar_t >() < that.value<wchar_t >() );
      case  4: return( value<int8_t  >() < that.value<int8_t  >() );
      case  5: return( value<uint8_t >() < that.value<uint8_t >() );
      case  6: return( value<int16_t >() < that.value<int16_t >() );
      case  7: return( value<uint16_t>() < that.value<uint16_t>() );
      case  8: return( value<int32_t >() < that.value<int32_t >() );
      case  9: return( value<uint32_t>() < that.value<uint32_t>() );
      case 10: return( value<int64_t >() < that.value<int64_t >() );
      case 11: return( value<uint64_t>() < that.value<uint64_t>() );
      case 12: return( value<float   >() < that.value<float   >() );
      case 13: return( value<double  >() < that.value<double  >() );
      case 15: size_t i(0);
        for(; i<size() && i<that.size(); i++)
          if( std::string::operator[](i) >= ((std::string)that).operator[](i) )
            return false;
        if(i<=size()) return true;
        for(; i<that.size(); i++) if(((std::string)that)[i]) return true;
    } return false;
  }

  untyped& untyped::operator+=( untyped const &that ) {
    switch( _type ?this->_type :that._type ) {
      case  1: operator=( static_cast<bool    > ( value<bool    >() + that.value<bool    >() ) ); break;
      case  2: operator=( static_cast<char    > ( value<char    >() + that.value<char    >() ) ); break;
      case  3: operator=( static_cast<wchar_t > ( value<wchar_t >() + that.value<wchar_t >() ) ); break;
      case  4: operator=( static_cast<int8_t  > ( value<int8_t  >() + that.value<int8_t  >() ) ); break;
      case  5: operator=( static_cast<uint8_t > ( value<uint8_t >() + that.value<uint8_t >() ) ); break;
      case  6: operator=( static_cast<int16_t > ( value<int16_t >() + that.value<int16_t >() ) ); break;
      case  7: operator=( static_cast<uint16_t> ( value<uint16_t>() + that.value<uint16_t>() ) ); break;
      case  8: operator=( static_cast<int32_t > ( value<int32_t >() + that.value<int32_t >() ) ); break;
      case  9: operator=( static_cast<uint32_t> ( value<uint32_t>() + that.value<uint32_t>() ) ); break;
      case 10: operator=( static_cast<int64_t > ( value<int64_t >() + that.value<int64_t >() ) ); break;
      case 11: operator=( static_cast<uint64_t> ( value<uint64_t>() + that.value<uint64_t>() ) ); break;
      case 12: operator=( static_cast<float   > ( value<float   >() + that.value<float   >() ) ); break;
      case 13: operator=( static_cast<double  > ( value<double  >() + that.value<double  >() ) ); break;
      case 15: if(( _type ?this->_type :that._type )==that._type) std::string::append( that ); else std::string::push_back( that ); break;
    } _set(vectorType(that)); _set(mapType(that));
    return *this;
  }

  untyped& untyped::operator-=( untyped const &that ) {
    switch( this->_type ) {
      case  1: operator=( static_cast<bool    > ( value<bool    >() - that.value<bool     >() ) ); break;
      case  2: operator=( static_cast<char    > ( value<char    >() - that.value<char     >() ) ); break;
      case  3: operator=( static_cast<wchar_t > ( value<wchar_t >() - that.value<wchar_t  >() ) ); break;
      case  4: operator=( static_cast<int8_t  > ( value<int8_t  >() - that.value<int8_t   >() ) ); break;
      case  5: operator=( static_cast<uint8_t > ( value<uint8_t >() - that.value<uint8_t  >() ) ); break;
      case  6: operator=( static_cast<int16_t > ( value<int16_t >() - that.value<int16_t  >() ) ); break;
      case  7: operator=( static_cast<uint16_t> ( value<uint16_t>() - that.value<uint16_t >() ) ); break;
      case  8: operator=( static_cast<int32_t > ( value<int32_t >() - that.value<int32_t  >() ) ); break;
      case  9: operator=( static_cast<uint32_t> ( value<uint32_t>() - that.value<uint32_t >() ) ); break;
      case 10: operator=( static_cast<int64_t > ( value<int64_t >() - that.value<int64_t  >() ) ); break;
      case 11: operator=( static_cast<uint64_t> ( value<uint64_t>() - that.value<uint64_t >() ) ); break;
      case 12: operator=( static_cast<float   > ( value<float   >() - that.value<float    >() ) ); break;
      case 13: operator=( static_cast<double  > ( value<double  >() - that.value<double   >() ) ); break;
      case 15: auto i=std::string::find(that); if(i!=npos) std::string::erase( i, (that._type==_type ?that.size() :1) );
    } return *this;
  }

  untyped& untyped::operator*=( untyped const &that ) {
    switch( this->_type ) {
      case  1: operator=( static_cast<bool    > ( value<bool    >()&& that.value<bool    >() ) ); break;
      case  2: operator=( static_cast<char    > ( value<char    >() * that.value<char    >() ) ); break;
      case  3: operator=( static_cast<wchar_t > ( value<wchar_t >() * that.value<wchar_t >() ) ); break;
      case  4: operator=( static_cast<int8_t  > ( value<int8_t  >() * that.value<int8_t  >() ) ); break;
      case  5: operator=( static_cast<uint8_t > ( value<uint8_t >() * that.value<uint8_t >() ) ); break;
      case  6: operator=( static_cast<int16_t > ( value<int16_t >() * that.value<int16_t >() ) ); break;
      case  7: operator=( static_cast<uint16_t> ( value<uint16_t>() * that.value<uint16_t>() ) ); break;
      case  8: operator=( static_cast<int32_t > ( value<int32_t >() * that.value<int32_t >() ) ); break;
      case  9: operator=( static_cast<uint32_t> ( value<uint32_t>() * that.value<uint32_t>() ) ); break;
      case 10: operator=( static_cast<int64_t > ( value<int64_t >() * that.value<int64_t >() ) ); break;
      case 11: operator=( static_cast<uint64_t> ( value<uint64_t>() * that.value<uint64_t>() ) ); break;
      case 12: operator=( static_cast<float   > ( value<float   >() * that.value<float   >() ) ); break;
      case 13: operator=( static_cast<double  > ( value<double  >() * that.value<double  >() ) ); break;
      case 15:{
        std::string::clear();
        for( size_t i(0), len=size(); i<len; i++,i++ )
          std::string::insert( i, 1, ((std::string)that).at(i) );
        }break;
      default: clear();
    } return *this;
  }

  untyped& untyped::operator/=( untyped const &that ) {
    switch( this->_type ) {
      case  1: operator=( static_cast<bool    > ( value<bool    >() / that.value<bool    >() ) ); break;
      case  2: operator=( static_cast<char    > ( value<char    >() / that.value<char    >() ) ); break;
      case  3: operator=( static_cast<wchar_t > ( value<wchar_t >() / that.value<wchar_t >() ) ); break;
      case  4: operator=( static_cast<int8_t  > ( value<int8_t  >() / that.value<int8_t  >() ) ); break;
      case  5: operator=( static_cast<uint8_t > ( value<uint8_t >() / that.value<uint8_t >() ) ); break;
      case  6: operator=( static_cast<int16_t > ( value<int16_t >() / that.value<int16_t >() ) ); break;
      case  7: operator=( static_cast<uint16_t> ( value<uint16_t>() / that.value<uint16_t>() ) ); break;
      case  8: operator=( static_cast<int32_t > ( value<int32_t >() / that.value<int32_t >() ) ); break;
      case  9: operator=( static_cast<uint32_t> ( value<uint32_t>() / that.value<uint32_t>() ) ); break;
      case 10: operator=( static_cast<int64_t > ( value<int64_t >() / that.value<int64_t >() ) ); break;
      case 11: operator=( static_cast<uint64_t> ( value<uint64_t>() / that.value<uint64_t>() ) ); break;
      case 12: operator=( static_cast<float   > ( value<float   >() / that.value<float   >() ) ); break;
      case 13: operator=( static_cast<double  > ( value<double  >() / that.value<double  >() ) ); break;
    } return *this;
  }

  untyped& untyped::operator%=( untyped const &that ) {
    switch( this->_type ) {
      case  1: operator=( static_cast<bool    > ( value<bool>()     % that.value<bool    >() ) ); break;
      case  2: operator=( static_cast<char    > ( value<char>()     % that.value<char    >() ) ); break;
      case  3: operator=( static_cast<wchar_t > ( value<wchar_t>()  % that.value<wchar_t >() ) ); break;
      case  4: operator=( static_cast<int8_t  > ( value<int8_t>()   % that.value<int8_t  >() ) ); break;
      case  5: operator=( static_cast<uint8_t > ( value<uint8_t>()  % that.value<uint8_t >() ) ); break;
      case  6: operator=( static_cast<int16_t > ( value<int16_t>()  % that.value<int16_t >() ) ); break;
      case  7: operator=( static_cast<uint16_t> ( value<uint16_t>() % that.value<uint16_t>() ) ); break;
      case  8: operator=( static_cast<int32_t > ( value<int32_t>()  % that.value<int32_t >() ) ); break;
      case  9: operator=( static_cast<uint32_t> ( value<uint32_t>() % that.value<uint32_t>() ) ); break;
      case 10: operator=( static_cast<int64_t > ( value<int64_t>()  % that.value<int64_t >() ) ); break;
      case 11: operator=( static_cast<uint64_t> ( value<uint64_t>() % that.value<uint64_t>() ) ); break;
    } return *this;
  }

  untyped& untyped::operator&=( untyped const &that ) {
    switch( this->_type ) {
      case  1: operator=( static_cast<bool    > ( value<bool    >() & that.value<bool    >() ) ); break;
      case  2: operator=( static_cast<char    > ( value<char    >() & that.value<char    >() ) ); break;
      case  3: operator=( static_cast<wchar_t > ( value<wchar_t >() & that.value<wchar_t >() ) ); break;
      case  4: operator=( static_cast<int8_t  > ( value<int8_t  >() & that.value<int8_t  >() ) ); break;
      case  5: operator=( static_cast<uint8_t > ( value<uint8_t >() & that.value<uint8_t >() ) ); break;
      case  6: operator=( static_cast<int16_t > ( value<int16_t >() & that.value<int16_t >() ) ); break;
      case  7: operator=( static_cast<uint16_t> ( value<uint16_t>() & that.value<uint16_t>() ) ); break;
      case  8: operator=( static_cast<int32_t > ( value<int32_t >() & that.value<int32_t >() ) ); break;
      case  9: operator=( static_cast<uint32_t> ( value<uint32_t>() & that.value<uint32_t>() ) ); break;
      case 10: operator=( static_cast<int64_t > ( value<int64_t >() & that.value<int64_t >() ) ); break;
      case 11: operator=( static_cast<uint64_t> ( value<uint64_t>() & that.value<uint64_t>() ) ); break;
    } return *this;
  }

  untyped& untyped::operator|=( untyped const &that ) {
    switch( this->_type ) {
      case  1: operator=( static_cast<bool    > ( value<bool    >() | that.value<bool    >() ) ); break;
      case  2: operator=( static_cast<char    > ( value<char    >() | that.value<char    >() ) ); break;
      case  3: operator=( static_cast<wchar_t > ( value<wchar_t >() | that.value<wchar_t >() ) ); break;
      case  4: operator=( static_cast<int8_t  > ( value<int8_t  >() | that.value<int8_t  >() ) ); break;
      case  5: operator=( static_cast<uint8_t > ( value<uint8_t >() | that.value<uint8_t >() ) ); break;
      case  6: operator=( static_cast<int16_t > ( value<int16_t >() | that.value<int16_t >() ) ); break;
      case  7: operator=( static_cast<uint16_t> ( value<uint16_t>() | that.value<uint16_t>() ) ); break;
      case  8: operator=( static_cast<int32_t > ( value<int32_t >() | that.value<int32_t >() ) ); break;
      case  9: operator=( static_cast<uint32_t> ( value<uint32_t>() | that.value<uint32_t>() ) ); break;
      case 10: operator=( static_cast<int64_t > ( value<int64_t >() | that.value<int64_t >() ) ); break;
      case 11: operator=( static_cast<uint64_t> ( value<uint64_t>() | that.value<uint64_t>() ) ); break;
    } return *this;
  }

  std::ostream& operator<<( std::ostream &out, untyped const &that ) {
    if ( that.isJsonMode() && ((untyped::mapType)that).size() ) {
      out << ((untyped::mapType)that);
      return out;
    }if( that.isJsonMode() && ((untyped::vectorType)that).size() ){
      out << ((untyped::vectorType)that);
      return out;
    }switch( that._type ) {
      case  1:
        if( that.isJsonMode() )
              out << (that.value<bool>() ?"true" :"false");
        else {out << static_cast<bool>(that.value<bool>() );}         break;
      case  2:
        if( that.isJsonMode() ) {out << '\'';};
        out << static_cast<char>(that.value<char>() );
        if( that.isJsonMode() ) {out << '\'';};                       break;
      case  3:
        if( that.isJsonMode() ) {out << '\'';};
        out << static_cast<wchar_t >(that.value<wchar_t >() );
        if( that.isJsonMode() ) {out << '\'';};                       break;
      case  4: out << static_cast<int8_t  >(that.value<int8_t  >() ); break;
      case  5: out << static_cast<uint8_t >(that.value<uint8_t >() ); break;
      case  6: out << static_cast<int16_t >(that.value<int16_t >() ); break;
      case  7: out << static_cast<uint16_t>(that.value<uint16_t>() ); break;
      case  8: out << static_cast<int32_t >(that.value<int32_t >() ); break;
      case  9: out << static_cast<uint32_t>(that.value<uint32_t>() ); break;
      case 10: out << static_cast<int64_t >(that.value<int64_t >() ); break;
      case 11: out << static_cast<uint64_t>(that.value<uint64_t>() ); break;
      case 12: out << static_cast<float   >(that.value<float   >() ); break;
      case 13: out << static_cast<double  >(that.value<double  >() ); break;
      case 15:
        if( that.isJsonMode() ) {out << '"';};
        out.write( that.data(), that.size() );
        if( that.isJsonMode() ) {out << '"';};
        break;
      default: if( that.isJsonMode() ) out.write( "null", 4 );
    } return out;
  }

  std::ostream& operator<< ( std::ostream &out, untyped::mapType const &that ) {
    out  << '{'; untyped::_jsonINCR();   untyped::_jsonNL(out);
    for( untyped::mapType::const_iterator it=that.begin(); it!=that.end(); ) {
      untyped::_jsonTAB(out);
      out << '\"' << it->first << "\":"; untyped::_jsonSP(out);
      out << (it->second);
      if(++it!=that.end())  {out << ','; untyped::_jsonNL(out);}
    } untyped::_jsonNL(out); untyped::_jsonDECR(); untyped::_jsonTAB(out);
    out << '}';
    return out;
  }

  std::ostream& operator<< ( std::ostream &out, untyped::vectorType const &that ) {
    out << '['; untyped::_jsonINCR();    untyped::_jsonNL(out);
    for( untyped::vectorType::const_iterator it=that.begin(); it!=that.end(); ) {
      untyped::_jsonTAB(out);
      out << *it;
      if(++it!=that.end())  {out << ','; untyped::_jsonNL(out);}
    } untyped::_jsonNL(out); untyped::_jsonDECR(); untyped::_jsonTAB(out);
    out << ']';
    return out;
  }

  void untyped::_writeTypeAndStructure( std::ostream &out, uchar type, uchar const &meta ) {
    type=((type<<3) | (meta&7));
    out.write( reinterpret_cast<char*>(&type), sizeof(uchar) );
  }

  untyped& untyped::serialize( std::ostream &out ) { if( isJsonMode() ) return serializeJson( out );
    char meta(0);
    if( vectorSize()) {
      if( mapSize() )     meta=1;
      else                meta=2;
    }else if( mapSize() ) meta=3;
    _writeTypeAndStructure( out, this->_type, meta );
    switch( this->_type ) {
      case  1: out.write( hton( value<bool    >() ), sizeof(bool    ) ); break;
      case  2: out.write( hton( value<char    >() ), sizeof(char    ) ); break;
      case  3: out.write( hton( value<wchar_t >() ), sizeof(wchar_t ) ); break;
      case  4: out.write( hton( value<int8_t  >() ), sizeof(int8_t  ) ); break;
      case  5: out.write( hton( value<uint8_t >() ), sizeof(uint8_t ) ); break;
      case  6: out.write( hton( value<int16_t >() ), sizeof(int16_t ) ); break;
      case  7: out.write( hton( value<uint16_t>() ), sizeof(uint16_t) ); break;
      case  8: out.write( hton( value<int32_t >() ), sizeof(int32_t ) ); break;
      case  9: out.write( hton( value<uint32_t>() ), sizeof(uint32_t) ); break;
      case 10: out.write( hton( value<int64_t >() ), sizeof(int64_t ) ); break;
      case 11: out.write( hton( value<uint64_t>() ), sizeof(uint64_t) ); break;
      case 12: out.write( hton( value<float   >() ), sizeof(float   ) ); break;
      case 13: out.write( hton( value<double  >() ), sizeof(double  ) ); break;
      case 15: _writeSize( out, this->size() ); out.write( this->data(), this->size() );
    }if ( meta == 1 || meta == 2 ) {
      _writeSize( out, this->vectorSize() );
      for( auto x : vector() )
        x.serialize( out );
    }if ( meta == 1 || meta == 3 ) {
      _writeSize( out, this->mapSize() );
      for( auto x : map() ) {
        untyped( x.first ).serialize( out );
        x.second.serialize( out );
    } }
    return *this;
  }

  unsigned char untyped::_readTypeAndStructure( std::istream &in, uchar &meta ) {
    uchar type(0);
    if( in.read( reinterpret_cast<char*>(&type), 1 ) ) {
      meta = type & 7; type = type >> 3;
    }return type;
  }

  untyped& untyped::deserialize( std::istream &in ){ if( isJsonMode() ) return deserializeJson( in );
    char c; uchar meta;
    size_t len;
    std::string::clear();
    switch( (_type=_readTypeAndStructure( in, meta )) ) {
      case  1: for(size_t i(0); i<sizeof(bool    ) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case  2: for(size_t i(0); i<sizeof(char    ) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case  3: for(size_t i(0); i<sizeof(wchar_t ) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case  4: for(size_t i(0); i<sizeof(int8_t  ) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case  5: for(size_t i(0); i<sizeof(uint8_t ) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case  6: for(size_t i(0); i<sizeof(int16_t ) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case  7: for(size_t i(0); i<sizeof(uint16_t) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case  8: for(size_t i(0); i<sizeof(int32_t ) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case  9: for(size_t i(0); i<sizeof(uint32_t) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case 10: for(size_t i(0); i<sizeof(int64_t ) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case 11: for(size_t i(0); i<sizeof(uint64_t) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case 12: for(size_t i(0); i<sizeof(float   ) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case 13: for(size_t i(0); i<sizeof(double  ) && in.read(&c, sizeof(char)); i++ ) std::string::operator+=(c); ntoh( *(std::string*)this ); break;
      case 15: _readSize( in, len ); while( len--  && in.read(&c, sizeof(char)) ) std::string::operator+=(c); break;
    }switch( meta ) {
      case 1: clearVector(); _readSize(in, len); while(len--)  vector().push_back( untyped().deserialize( in ) );                                 // read dataVector...
      case 3: clearMap();    _readSize(in, len); while(len--) {untyped d; d.deserialize( in ); operator[](d)=untyped().deserialize( in );} break; // read dataMap
      case 2: clearVector(); _readSize(in, len); while(len--)  vector().push_back( untyped().deserialize( in ) );                                 // read dataVector
    }return *this;
  }

  void untyped::_getJsonComment( std::istream &in, char &c ) {
    bool ml, exit(false);
    if( in.read( &c, 1 ) && ((ml=(c=='*')) || c=='/') ) {
      while ( c && in.read( &c, 1 ) ) switch( c ) {
        case '\n': if( !ml ) return; exit=false;  break;
        case '*' : if( ml ) exit=true;            break;
        case '/' : if(exit) return;               break;
        default  : exit=false;
    } }
    else c='\0';
  }

  void untyped::_getJsonNull( std::istream &in, char &c ) {
    bool stop(false); untyped ret("");
    do switch( c ) {
      case '}' :
      case ']' :
      case ',' : stop=true;
        break;
      default  : if(_isWhiteSpace(c)) stop=true; else ret+=tolower(c);
    }while ( !stop && in.read( &c, 1 ) );
    if( ret != "null" ) c='\0';
  }

  untyped untyped::_getJsonBool( std::istream &in, char &c ) {
    bool stop(false); untyped ret("");
    do switch( c ) {
        case '}' :
        case ']' :
        case ',' : stop=true;
          break;
        default  : if ( _isWhiteSpace(c) ) stop=true; else ret+=tolower(c);
    }while ( !stop && in.read( &c, 1 ) );
    if( ret=="true" )   return true;
    if( ret!="false" )  c='\0';
    return false;
  }

  untyped untyped::_getJsonNumber( std::istream &in, char &c ) {
    untyped ret("");
    bool    stop(false), neg(false), point(false), exp(false);
    do switch( c ) {
      case '+' :
        if(ret.size() && ret[ret.size()-1]!='e') {c='\0';};
        break;
      case '-' :
        if( (ret.size() || neg) && ret[ret.size()-1]=='e' ) {c='\0';} else {ret+='-';};
        neg=true;   break;
      case '.' : if(point || exp) {c='\0';} else {ret+=c;};
        point=true; break;
      case 'e' :
      case 'E' : if(exp) {c='\0';} else {ret+='e';};
        exp=true;   break;
      case '}' :
      case ']' :
      case ',' :
        stop=true;  break;
      default  :
        if(_isWhiteSpace(c)) stop=true; else ret+=c;
    }while( !stop && c && in.read( &c, 1 ) );
    if (c) {
      if(point || exp) ret = atof(ret.c_str());
      else             ret = (int64_t)atol(ret.c_str());
      return ret;
    }return ret.clear();
  }

  untyped untyped::_getJsonChar( std::istream &in, char &c ) {
    char ret('\0');
    if( c!='\'' ) c='\0';
    if( !in.read( &c, 1 ) || (!isgraph(c) && c>'\xA0') ) c='\0'; else ret=c;
    if( !in.read( &c, 1 ) || c!='\'' ) c='\0';
    return( c ? untyped(ret) : untyped() );
  }

  untyped untyped::_getJsonString( std::istream &in, char &c ) {
    untyped ret("");
    for (bool stop(false); !stop && in.read( &c, 1 ); ) switch( c ) {
      case '"' : stop=true;
        break;
      default  : if(isgraph(c) || c>='\xA0') ret+=c;
    } return(c ?ret :untyped() );
  }

  untyped untyped::_getJsonValue( std::istream &in, char &c ) {
    do switch( c ) {
      case '{' : return _getJsonObject(in, c);
      case '[' : return _getJsonArray(in, c);
      case '"' : return _getJsonString(in, c);
      case 't' :
      case 'T' :
      case 'f' :
      case 'F' : return _getJsonBool(in, c);
      case 'n' :
      case 'N' : _getJsonNull(in, c); return untyped();
      case '\'': return _getJsonChar(in, c);
      case '/' : _getJsonComment(in, c); break;
      default  : if(!_isWhiteSpace(c)) return _getJsonNumber(in, c);
    }while ( c && in.read( &c, 1 ) );
    return untyped();
  }

  untyped untyped::_getJsonArray( std::istream &in, char &c ) {
    untyped ret, x;
    while ( c && c!=']' && in.read(&c, 1) ) switch(c) {
      case ']' : break;
      case ',' : break;
      case '/' : _getJsonComment(in, c); break;
      default  : if( !_isWhiteSpace(c) ) {
        x =_getJsonValue(in, c); ret[ret.vectorSize()] = x; x.clear();
    } }
    return( c ?ret.clearValue() :untyped() );
  }

  untyped untyped::_getJsonObject( std::istream &in, char &c ) {
    uchar next(1); std::string s; untyped ret;
    while ( c && c!='}' && in.read( &c, 1 ) ) switch( c ) {
      case '}' : break;
      case ',' : if ( next++ ) c='\0';
        break;
      case '"' :
        if( next == 3 ) {
          ret[s] = _getJsonString(in, c);
          next=0; if( c==',' ) next++;
        }else if( next++!=1 ) c='\0';
        else  s = _getJsonString(in, c);
        break;
      case ':' : if(next++!=2) c='\0';
        break;
      case '[' :
        if ( next!=3 ) c='\0';
        else {
          ret[s] = _getJsonArray(in, c);
          next=0;
        } break;
      case '/' : _getJsonComment(in, c); break;
      default  :
        if( !_isWhiteSpace(c) ) {
          if ( next!=3 ) c='\0';
          else {
            ret[s] = _getJsonValue(in, c);
            next=0; if( c==',' ) next++;
    }   } }
    return( c ?ret :untyped() );
  }

  untyped& untyped::deserializeJson( std::istream &in ) { //See: https://www.json.org/json-fr.html
    char c; clear(); while( in.read( &c, 1 ) ) switch(c) {
      case '{': (*this) = _getJsonObject(in, c); if(c!='}') clear(); return *this;
      case '[': (*this) = _getJsonArray (in, c); if(c!=']') clear(); return *this;
      case '/': _getJsonComment(in, c);          if(  !c  ) clear(); return *this;
      default : if( !_isWhiteSpace(c) )                              return *this;
    }while(in.read(&c, 1));
    return *this;
  }

}

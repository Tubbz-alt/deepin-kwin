/*****************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2004 Lubos Lunak <l.lunak@kde.org>

You can Freely distribute this program under the GNU General Public
License. See the file "COPYING" for the exact licensing terms.
******************************************************************/

#include "rules.h"

#include <fixx11h.h>
#include <kconfig.h>
#include <qregexp.h>
#include <ktempfile.h>
#include <ksimpleconfig.h>
#include <qfile.h>

#include "client.h"
#include "workspace.h"

namespace KWinInternal
{

Rules::Rules()
    : temporary_state( 0 )
    , wmclassregexp( false )
    , wmclasscomplete( false )
    , windowroleregexp( false )
    , titleregexp( false )
    , extraroleregexp( false )
    , clientmachineregexp( false )
    , types( NET::AllTypesMask )
    , placementrule( UnusedForceRule )
    , positionrule( UnusedSetRule )
    , sizerule( UnusedSetRule )
    , minsizerule( UnusedForceRule )
    , maxsizerule( UnusedForceRule )
    , ignorepositionrule( UnusedForceRule )
    , desktoprule( UnusedSetRule )
    , typerule( UnusedForceRule )
    , maximizevertrule( UnusedSetRule )
    , maximizehorizrule( UnusedSetRule )
    , minimizerule( UnusedSetRule )
    , shaderule( UnusedSetRule )
    , skiptaskbarrule( UnusedSetRule )
    , skippagerrule( UnusedSetRule )
    , aboverule( UnusedSetRule )
    , belowrule( UnusedSetRule )
    , fullscreenrule( UnusedSetRule )
    , noborderrule( UnusedSetRule )
    , fsplevelrule( UnusedForceRule )
    , acceptfocusrule( UnusedForceRule )
    , moveresizemoderule( UnusedForceRule )
    , closeablerule( UnusedForceRule )
    {
    }

Rules::Rules( const QString& str, bool temporary )
    : temporary_state( temporary ? 2 : 0 )
    {
    KTempFile file;
    QFile* f = file.file();
    if( f != NULL )
        {
        QCString s = str.utf8();
        f->writeBlock( s.data(), s.length());
        }
    file.close();
    KSimpleConfig cfg( file.name());
    readFromCfg( cfg );
    file.unlink();
    }

#define READ_MATCH_STRING( var, func ) \
    var = cfg.readEntry( #var ) func; \
    var##regexp = cfg.readBoolEntry( #var "regexp" );
    
#define READ_SET_RULE( var, type, func ) \
    var = func ( cfg.read##type##Entry( #var )); \
    var##rule = readSetRule( cfg, #var "rule" );
    
#define READ_SET_RULE_DEF( var, type, func, def ) \
    var = func ( cfg.read##type##Entry( #var, def )); \
    var##rule = readSetRule( cfg, #var "rule" );
    
#define READ_SET_RULE_2( var, type, func, funcarg ) \
    var = func ( cfg.read##type##Entry( #var ), funcarg ); \
    var##rule = readSetRule( cfg, #var "rule" );

#define READ_FORCE_RULE( var, type, func ) \
    var = func ( cfg.read##type##Entry( #var )); \
    var##rule = readForceRule( cfg, #var "rule" );

#define READ_FORCE_RULE_2( var, type, func, funcarg ) \
    var = func ( cfg.read##type##Entry( #var ), funcarg ); \
    var##rule = readForceRule( cfg, #var "rule" );


Rules::Rules( KConfig& cfg )
    : temporary_state( 0 )
    {
    readFromCfg( cfg );
    }

static int limit0to4( int i ) { return QMAX( 0, QMIN( 4, i )); }

void Rules::readFromCfg( KConfig& cfg )
    {
    wmclass = cfg.readEntry( "wmclass" ).lower().latin1();
    wmclassregexp = cfg.readBoolEntry( "wmclassregexp" );
    wmclasscomplete = cfg.readBoolEntry( "wmclasscomplete" );
    READ_MATCH_STRING( windowrole, .lower().latin1() );
    READ_MATCH_STRING( title, );
    READ_MATCH_STRING( extrarole, .lower().latin1() );
    READ_MATCH_STRING( clientmachine, .lower().latin1() );
    types = cfg.readUnsignedLongNumEntry( "types", NET::AllTypesMask );
    READ_FORCE_RULE_2( placement,, Placement::policyFromString, false );
    READ_SET_RULE_DEF( position, Point,, &invalidPoint );
    READ_SET_RULE( size, Size, );
    if( size.isEmpty() && sizerule != ( SetRule )Remember)
        sizerule = UnusedSetRule;
    READ_FORCE_RULE( minsize, Size, );
    if( !minsize.isValid())
        minsizerule = UnusedForceRule;
    READ_FORCE_RULE( maxsize, Size, );
    if( maxsize.isEmpty())
        maxsizerule = UnusedForceRule;
    READ_FORCE_RULE( ignoreposition, Bool, );
    READ_SET_RULE( desktop, Num, );
    type = readType( cfg, "type" );
    typerule = type != NET::Unknown ? readForceRule( cfg, "typerule" ) : UnusedForceRule;
    READ_SET_RULE( maximizevert, Bool, );
    READ_SET_RULE( maximizehoriz, Bool, );
    READ_SET_RULE( minimize, Bool, );
    READ_SET_RULE( shade, Bool, );
    READ_SET_RULE( skiptaskbar, Bool, );
    READ_SET_RULE( skippager, Bool, );
    READ_SET_RULE( above, Bool, );
    READ_SET_RULE( below, Bool, );
    READ_SET_RULE( fullscreen, Bool, );
    READ_SET_RULE( noborder, Bool, );
    READ_FORCE_RULE( fsplevel, Num, limit0to4 ); // fsp is 0-4
    READ_FORCE_RULE( acceptfocus, Bool, );
    READ_FORCE_RULE( moveresizemode, , Options::stringToMoveResizeMode );
    READ_FORCE_RULE( closeable, Bool, );
    kdDebug() << "READ RULE:" << this << endl;
    }

#undef READ_MATCH_STRING
#undef READ_SET_RULE
#undef READ_SET_RULE_2
#undef READ_FORCE_RULE
#undef READ_FORCE_RULE_2

#define WRITE_MATCH_STRING( var, cast ) \
    if( !var.isEmpty()) \
        { \
        cfg.writeEntry( #var, cast var ); \
        cfg.writeEntry( #var "regexp", var##regexp ); \
        } \
    else \
        { \
        cfg.deleteEntry( #var ); \
        cfg.deleteEntry( #var "regexp" ); \
        }

#define WRITE_SET_RULE( var, func ) \
    if( var##rule != UnusedSetRule ) \
        { \
        cfg.writeEntry( #var, func ( var )); \
        cfg.writeEntry( #var "rule", var##rule ); \
        } \
    else \
        { \
        cfg.deleteEntry( #var ); \
        cfg.deleteEntry( #var "rule" ); \
        }

#define WRITE_FORCE_RULE( var, func ) \
    if( var##rule != UnusedForceRule ) \
        { \
        cfg.writeEntry( #var, func ( var )); \
        cfg.writeEntry( #var "rule", var##rule ); \
        } \
    else \
        { \
        cfg.deleteEntry( #var ); \
        cfg.deleteEntry( #var "rule" ); \
        }

#define WRITE_WITH_DEFAULT( var, default ) \
    if( var != default ) \
        cfg.writeEntry( #var, var ); \
    else \
        cfg.deleteEntry( #var );


void Rules::write( KConfig& cfg ) const
    {
    // always write wmclass
    cfg.writeEntry( "wmclass", ( const char* )wmclass );
    cfg.writeEntry( "wmclassregexp", wmclassregexp );
    cfg.writeEntry( "wmclasscomplete", wmclasscomplete );
    WRITE_MATCH_STRING( windowrole, (const char*) );
    WRITE_MATCH_STRING( title, );
    WRITE_MATCH_STRING( extrarole, (const char*) );
    WRITE_MATCH_STRING( clientmachine, (const char*) );
    WRITE_WITH_DEFAULT( types, NET::AllTypesMask );
    WRITE_FORCE_RULE( placement, Placement::policyToString );
    WRITE_SET_RULE( position, );
    WRITE_SET_RULE( size, );
    WRITE_FORCE_RULE( minsize, );
    WRITE_FORCE_RULE( maxsize, );
    WRITE_FORCE_RULE( ignoreposition, );
    WRITE_SET_RULE( desktop, );
    WRITE_FORCE_RULE( type, );
    WRITE_SET_RULE( maximizevert, );
    WRITE_SET_RULE( maximizehoriz, );
    WRITE_SET_RULE( minimize, );
    WRITE_SET_RULE( shade, );
    WRITE_SET_RULE( skiptaskbar, );
    WRITE_SET_RULE( skippager, );
    WRITE_SET_RULE( above, );
    WRITE_SET_RULE( below, );
    WRITE_SET_RULE( fullscreen, );
    WRITE_SET_RULE( noborder, );
    WRITE_FORCE_RULE( fsplevel, );
    WRITE_FORCE_RULE( acceptfocus, );
    WRITE_FORCE_RULE( moveresizemode, Options::moveResizeModeToString );
    WRITE_FORCE_RULE( closeable, );
    }
    
#undef WRITE_MATCH_STRING
#undef WRITE_SET_RULE
#undef WRITE_FORCE_RULE
#undef WRITE_WITH_DEFAULT

Rules::SetRule Rules::readSetRule( KConfig& cfg, const QString& key )
    {
    int v = cfg.readNumEntry( key );
    if( v >= DontAffect && v <= Remember )
        return static_cast< SetRule >( v );
    return UnusedSetRule;
    }

Rules::ForceRule Rules::readForceRule( KConfig& cfg, const QString& key )
    {
    int v = cfg.readNumEntry( key );
    if( v == DontAffect || v == Force )
        return static_cast< ForceRule >( v );
    return UnusedForceRule;
    }

NET::WindowType Rules::readType( KConfig& cfg, const QString& key )
    {
    int v = cfg.readNumEntry( key );
    if( v >= NET::Normal && v <= NET::Splash )
        return static_cast< NET::WindowType >( v );
    return NET::Unknown;
    }

bool Rules::match( const Client* c ) const
    {
    if( types != NET::AllTypesMask )
        {
        NET::WindowType t = c->windowType( true ); // direct type
        if( t == NET::Unknown )
            t = NET::Normal; // NET::Unknown->NET::Normal is only here for matching
        if( !NET::typeMatchesMask( t, types ))
            return false;
        }
    if( !wmclass.isEmpty())
        { // TODO optimize?
        QCString cwmclass = wmclasscomplete
            ? c->resourceName() + ' ' + c->resourceClass() : c->resourceClass();
        if( wmclassregexp && QRegExp( wmclass ).search( cwmclass ) == -1 )
            return false;
        if( !wmclassregexp && wmclass != cwmclass )
            return false;
        }
    if( !windowrole.isEmpty())
        {
        if( windowroleregexp && QRegExp( windowrole ).search( c->windowRole()) == -1 )
            return false;
        if( !windowroleregexp && windowrole != c->windowRole())
            return false;
        }
    if( !title.isEmpty())
        {
        if( titleregexp && QRegExp( title ).search( c->caption( false )) == -1 )
            return false;
        if( !titleregexp && title != c->caption( false ))
            return false;
        }
    // TODO extrarole
    if( !clientmachine.isEmpty())
        {
        if( clientmachineregexp
            && QRegExp( clientmachine ).search( c->wmClientMachine( true )) == -1
            && QRegExp( clientmachine ).search( c->wmClientMachine( false )) == -1 )
            return false;
        if( !clientmachineregexp
            && clientmachine != c->wmClientMachine( true )
            && clientmachine != c->wmClientMachine( false ))
            return false;
        }
    return true;
    }

bool Rules::update( Client* c )
    {
    // TODO check this setting is for this client ?
    bool updated = false;
    if( positionrule == ( SetRule )Remember)
        {
        updated = updated || position != c->pos();
        position = c->pos();
        }
    if( sizerule == ( SetRule )Remember)
        {
        updated = updated || size != c->size();
        size = c->size();
        }
    if( desktoprule == ( SetRule )Remember)
        {
        updated = updated || desktop != c->desktop();
        desktop = c->desktop();
        }
    if( maximizevertrule == ( SetRule )Remember)
        {
        updated = updated || maximizevert != bool( c->maximizeMode() & MaximizeVertical );
        maximizevert = c->maximizeMode() & MaximizeVertical;
        }
    if( maximizehorizrule == ( SetRule )Remember)
        {
        updated = updated || maximizehoriz != bool( c->maximizeMode() & MaximizeHorizontal );
        maximizehoriz = c->maximizeMode() & MaximizeHorizontal;
        }
    if( minimizerule == ( SetRule )Remember)
        {
        updated = updated || minimize != c->isMinimized();
        minimize = c->isMinimized();
        }
    if( shaderule == ( SetRule )Remember)
        {
        updated = updated || ( shade != ( c->shadeMode() != ShadeNone ));
        shade = c->shadeMode() != ShadeNone;
        }
    if( skiptaskbarrule == ( SetRule )Remember)
        {
        updated = updated || skiptaskbar != c->skipTaskbar();
        skiptaskbar = c->skipTaskbar();
        }
    if( skippagerrule == ( SetRule )Remember)
        {
        updated = updated || skippager != c->skipPager();
        skippager = c->skipPager();
        }
    if( aboverule == ( SetRule )Remember)
        {
        updated = updated || above != c->keepAbove();
        above = c->keepAbove();
        }
    if( belowrule == ( SetRule )Remember)
        {
        updated = updated || below != c->keepBelow();
        below = c->keepBelow();
        }
    if( fullscreenrule == ( SetRule )Remember)
        {
        updated = updated || fullscreen != c->isFullScreen();
        fullscreen = c->isFullScreen();
        }
    if( noborderrule == ( SetRule )Remember)
        {
        updated = updated || noborder != c->isUserNoBorder();
        noborder = c->isUserNoBorder();
        }
    return updated;
    }

#define APPLY_RULE( var, name, type ) \
bool Rules::apply##name( type& arg, bool init ) const \
    { \
    if( checkSetRule( var##rule, init )) \
        arg = this->var; \
    return checkSetStop( var##rule ); \
    }

#define APPLY_FORCE_RULE( var, name, type ) \
bool Rules::apply##name( type& arg ) const \
    { \
    if( checkForceRule( var##rule )) \
        arg = this->var; \
    return checkForceStop( var##rule ); \
    }

APPLY_FORCE_RULE( placement, Placement, Placement::Policy )

bool Rules::applyGeometry( QRect& rect, bool init ) const
    {
    QPoint p = rect.topLeft();
    QSize s = rect.size();
    bool ret = false; // no short-circuiting
    if( applyPosition( p, init ))
        {
        rect.moveTopLeft( p );
        ret = true;
        }
    if( applySize( s, init ))
        {
        rect.setSize( s );
        ret = true;
        }
    return ret;
    }

bool Rules::applyPosition( QPoint& pos, bool init ) const
    {
    if( pos != invalidPoint && checkSetRule( positionrule, init ))
        pos = this->position;
    return checkSetStop( positionrule );
    }

bool Rules::applySize( QSize& s, bool init ) const
    {
    if( size.isValid() && checkSetRule( sizerule, init ))
        s = this->size;
    return checkSetStop( sizerule );
    }

APPLY_FORCE_RULE( minsize, MinSize, QSize )
APPLY_FORCE_RULE( maxsize, MaxSize, QSize )
APPLY_FORCE_RULE( ignoreposition, IgnorePosition, bool )
APPLY_RULE( desktop, Desktop, int )
APPLY_FORCE_RULE( type, Type, NET::WindowType )

bool Rules::applyMaximizeHoriz( MaximizeMode& mode, bool init ) const
    {
    if( checkSetRule( maximizehorizrule, init ))
        mode = static_cast< MaximizeMode >(( maximizehoriz ? MaximizeHorizontal : 0 ) | ( mode & MaximizeVertical ));
    return checkSetStop( maximizehorizrule );
    }

bool Rules::applyMaximizeVert( MaximizeMode& mode, bool init ) const
    {
    if( checkSetRule( maximizevertrule, init ))
        mode = static_cast< MaximizeMode >(( maximizevert ? MaximizeVertical : 0 ) | ( mode & MaximizeHorizontal ));
    return checkSetStop( maximizevertrule );
    }

APPLY_RULE( minimize, Minimize, bool )

bool Rules::applyShade( ShadeMode& sh, bool init ) const
    {
    if( checkSetRule( shaderule, init ))
        {
        if( !this->shade )
            sh = ShadeNone;
        if( this->shade && sh == ShadeNone )
            sh = ShadeNormal;
        }
    return checkSetStop( shaderule );
    }

APPLY_RULE( skiptaskbar, SkipTaskbar, bool )
APPLY_RULE( skippager, SkipPager, bool )
APPLY_RULE( above, KeepAbove, bool )
APPLY_RULE( below, KeepBelow, bool )
APPLY_RULE( fullscreen, FullScreen, bool )
APPLY_RULE( noborder, NoBorder, bool )
APPLY_FORCE_RULE( fsplevel, FSP, int )
APPLY_FORCE_RULE( acceptfocus, AcceptFocus, bool )
APPLY_FORCE_RULE( moveresizemode, MoveResizeMode, Options::MoveResizeMode )
APPLY_FORCE_RULE( closeable, Closeable, bool )

#undef APPLY_RULE
#undef APPLY_FORCE_RULE

bool Rules::isTemporary() const
    {
    return temporary_state > 0;
    }

bool Rules::discardTemporary( bool force )
    {
    if( temporary_state == 0 ) // not temporary
        return false;
    if( force || --temporary_state == 0 ) // too old
        {
        kdDebug() << "DISCARD:" << this << endl;
        delete this;
        return true;
        }
    return false;
    }

#ifndef NDEBUG
kdbgstream& operator<<( kdbgstream& stream, const Rules* r )
    {
    return stream << "[" << r->wmclass << "/" << r->windowrole << "/" << r->title << "]";
    }
#endif

void WindowRules::discardTemporary()
    {
    QValueVector< Rules* >::Iterator it2 = rules.begin();
    for( QValueVector< Rules* >::Iterator it = rules.begin();
         it != rules.end();
         )
        {
        if( (*it)->discardTemporary( false ))
            ++it;
        else
            {
            *it2++ = *it++;
            }
        }
    rules.erase( it2, rules.end());
    }

void WindowRules::update( Client* c )
    {
    bool updated = false;
    for( QValueVector< Rules* >::ConstIterator it = rules.begin();
         it != rules.end();
         ++it )
        if( (*it)->update( c )) // no short-circuiting here
            updated = true;
    if( updated )
        Workspace::self()->rulesUpdated();
    }

#define CHECK_RULE( rule, type ) \
type WindowRules::check##rule( type arg, bool init ) const \
    { \
    if( rules.count() == 0 ) \
        return arg; \
    type ret = arg; \
    for( QValueVector< Rules* >::ConstIterator it = rules.begin(); \
         it != rules.end(); \
         ++it ) \
        { \
        if( (*it)->apply##rule( ret, init )) \
            break; \
        } \
    return ret; \
    }

#define CHECK_FORCE_RULE( rule, type ) \
type WindowRules::check##rule( type arg ) const \
    { \
    if( rules.count() == 0 ) \
        return arg; \
    type ret = arg; \
    for( QValueVector< Rules* >::ConstIterator it = rules.begin(); \
         it != rules.end(); \
         ++it ) \
        { \
        if( (*it)->apply##rule( ret )) \
            break; \
        } \
    return ret; \
    }

CHECK_FORCE_RULE( Placement, Placement::Policy )

QRect WindowRules::checkGeometry( QRect rect, bool init ) const
    {
    return QRect( checkPosition( rect.topLeft(), init ), checkSize( rect.size(), init ));
    }

CHECK_RULE( Position, QPoint )
CHECK_RULE( Size, QSize )
CHECK_FORCE_RULE( MinSize, QSize )
CHECK_FORCE_RULE( MaxSize, QSize )
CHECK_FORCE_RULE( IgnorePosition, bool )
CHECK_RULE( Desktop, int )
CHECK_FORCE_RULE( Type, NET::WindowType )
CHECK_RULE( MaximizeVert, KDecorationDefines::MaximizeMode )
CHECK_RULE( MaximizeHoriz, KDecorationDefines::MaximizeMode )

KDecorationDefines::MaximizeMode WindowRules::checkMaximize( MaximizeMode mode, bool init ) const
    {
    bool vert = checkMaximizeVert( mode, init ) & MaximizeVertical;
    bool horiz = checkMaximizeHoriz( mode, init ) & MaximizeHorizontal;
    return static_cast< MaximizeMode >(( vert ? MaximizeVertical : 0 ) | ( horiz ? MaximizeHorizontal : 0 ));
    }

CHECK_RULE( Minimize, bool )
CHECK_RULE( Shade, ShadeMode )
CHECK_RULE( SkipTaskbar, bool )
CHECK_RULE( SkipPager, bool )
CHECK_RULE( KeepAbove, bool )
CHECK_RULE( KeepBelow, bool )
CHECK_RULE( FullScreen, bool )
CHECK_RULE( NoBorder, bool )
CHECK_FORCE_RULE( FSP, int )
CHECK_FORCE_RULE( AcceptFocus, bool )
CHECK_FORCE_RULE( MoveResizeMode, Options::MoveResizeMode )
CHECK_FORCE_RULE( Closeable, bool )

#undef CHECK_RULE
#undef CHECK_FORCE_RULE

// Client

void Client::setupWindowRules( bool ignore_temporary )
    {
    client_rules = workspace()->findWindowRules( this, ignore_temporary );
    // check only after getting the rules, because there may be a rule forcing window type
    if( isTopMenu()) // TODO cannot have restrictions
        client_rules = WindowRules();
    }

void Client::updateWindowRules()
    {
    client_rules.update( this );
    }

void Client::finishWindowRules()
    {
    updateWindowRules();
    client_rules = WindowRules();
    }

// Workspace

WindowRules Workspace::findWindowRules( const Client* c, bool ignore_temporary )
    {
    QValueVector< Rules* > ret;
    for( QValueList< Rules* >::Iterator it = rules.begin();
         it != rules.end();
         )
        {
        if( ignore_temporary && (*it)->isTemporary())
            {
            ++it;
            continue;
            }
        if( (*it)->match( c ))
            {
            Rules* rule = *it;
            kdDebug() << "RULE FOUND:" << rule << ":" << c << endl;
            if( rule->isTemporary())
                it = rules.remove( it );
            else
                ++it;
            ret.append( rule );
            continue;
            }
        ++it;
        }
    return WindowRules( ret );
    }

void Workspace::editWindowRules( Client* c )
    {
    // TODO
    }

void Workspace::loadWindowRules()
    {
    while( !rules.isEmpty())
        {
        delete rules.front();
        rules.pop_front();
        }
    KConfig cfg( "kwinrulesrc", true );
    cfg.setGroup( "General" );
    int count = cfg.readNumEntry( "count" );
    kdDebug() << "RULES:" << count << endl;
    for( int i = 1;
         i <= count;
         ++i )
        {
        cfg.setGroup( QString::number( i ));
        Rules* rule = new Rules( cfg );
        rules.append( rule );
        }
    }

void Workspace::writeWindowRules()
    {
    KConfig cfg( "kwinrulesrc" );
    cfg.setGroup( "General" );
    cfg.writeEntry( "count", rules.count());
    int i = 1;
    for( QValueList< Rules* >::ConstIterator it = rules.begin();
         it != rules.end();
         ++it )
        {
        if( (*it)->isTemporary())
            continue;
        cfg.setGroup( QString::number( i ));
        (*it)->write( cfg );
        ++i;
        }
    }

void Workspace::gotTemporaryRulesMessage( const QString& message )
    {
    bool was_temporary = false;
    for( QValueList< Rules* >::ConstIterator it = rules.begin();
         it != rules.end();
         ++it )
        if( (*it)->isTemporary())
            was_temporary = true;
    Rules* rule = new Rules( message, true );
    rules.prepend( rule ); // highest priority first
    if( !was_temporary )
        QTimer::singleShot( 60000, this, SLOT( cleanupTemporaryRules()));
    }

void Workspace::cleanupTemporaryRules()
    {
    kdDebug() << "CLEANUP" << endl;
    bool has_temporary = false;
    for( QValueList< Rules* >::Iterator it = rules.begin();
         it != rules.end();
         )
        {
        if( (*it)->discardTemporary( false ))
            it = rules.remove( it );
        else
            {
            if( (*it)->isTemporary())
                has_temporary = true;
            ++it;
            }
        }
    if( has_temporary )
        QTimer::singleShot( 60000, this, SLOT( cleanupTemporaryRules()));
    }

void Workspace::rulesUpdated()
    {
    rulesUpdatedTimer.start( 1000, true );
    }

} // namespace

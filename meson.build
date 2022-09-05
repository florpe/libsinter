project(
  'sinter'
  , ['c']
  , version: '0.0.1'
  , meson_version: '>= 0.42'
  )

libsinter = library(
  'sinter'
  , sources: [
    'lib/mount.h'
    , 'lib/mount.c'
    , 'lib/sinter.h'
    , 'lib/sinter.c'
    ]
  , c_args: ['-Wall']
  , link_args: ['-lcap']
  )

executable(
  'sinter'
  , sources: [
    'util/sinter.c'
    , 'lib/mount.h'
    , 'lib/mount.c'
    , 'lib/sinter.h'
    , 'lib/sinter.c'
    ]
  , c_args: ['-Wall']
  , link_args: ['-lcap']
  #  , link_with: libsinter
  )

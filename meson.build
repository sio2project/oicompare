project (
  'OI Compare',

  'cpp',

  default_options: ['cpp_std=c++20'],
  version: '0.1.0'
)

subdir ('third_party')

executable (
  'oicompare',

  'oicompare.cc',

  dependencies: [
    fmt_dep,
    mio_dep,
  ]
)

test (
  'Test',

  executable (
    'tester',

    'tester.cc',

    dependencies: [
      fmt_dep,
    ]
  )
)
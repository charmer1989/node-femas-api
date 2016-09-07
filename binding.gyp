{
  "targets": [
    {
      "target_name": "femas",
      "sources": [
        "femas.cc",
        "wrap_trader.cpp",
        "uv_trader.cpp"
      ],
      "libraries": [
        "..\\tradeapi\\USTPtraderapi.lib"
      ],
      "include_dirs": [".\\tradeapi\\", "<!(node -e \"require('nan')\")"]
    }
  ]
}

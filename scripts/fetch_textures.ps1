# Downloads public-domain NASA equirectangular textures into the user's texture dir.
$ErrorActionPreference = 'Stop'
$dir = Join-Path $env:LOCALAPPDATA 'Globe\textures'
New-Item -ItemType Directory -Force -Path $dir | Out-Null

# NOTE: verify current URLs at https://visibleearth.nasa.gov before running.
$urls = @{
  'day.jpg'   = 'https://eoimages.gsfc.nasa.gov/images/imagerecords/73000/73776/world.topo.bathy.200408.3x5400x2700.jpg'
  'night.jpg' = 'https://eoimages.gsfc.nasa.gov/images/imagerecords/79000/79765/dnb_land_ocean_ice.2012.5400x2700.jpg'
  'clouds.png'= 'https://eoimages.gsfc.nasa.gov/images/imagerecords/57000/57752/f5_1km_2012175_0700.sw.png'
}

foreach ($k in $urls.Keys) {
    $dst = Join-Path $dir $k
    Write-Host "Fetching $k -> $dst"
    Invoke-WebRequest -Uri $urls[$k] -OutFile $dst
}
Write-Host "Done. Textures in $dir"


args = commandArgs(trailingOnly=TRUE)

if (length(args) < 3) stop("At least two argument must be supplied.", call.=FALSE)
infile = args[1]

nlibpath  = args[2]
nlibs = strsplit(nlibpath, ",")
nlibs = nlibs[[1]]

if (length(args) >= 3) {
  outfname = args[3]
} else {
  outfname = paste(basename(imagefile), ".nblust", sep="")
}

if (length(args) >= 4) {
	outputdir = args[4]
} else {
	outputdir = dirname(imagefile)
}
if (!dir.exists(outputdir)) {
    dir.create(outputdir, FALSE)
}

if (length(args) >= 5) {
	resultnum = strtoi(args[5])
	if (is.na(resultnum)) resultnum = 10
} else {
  resultnum = 10
}

if (length(args) >= 6) {
  dbnames = args[6]
} else {
  dbnames = paste(basename(nlibs), collapse=",")
}

if (length(args) >= 7) {
  normalization = args[7]
} else {
  normalization = "mean"
}

cat("Loading NAT...")

if (!require("nat",character.only = TRUE)) {
  install.packages("nat", repos="http://cran.rstudio.com/")
}
if (!require("nat.nblast",character.only = TRUE)) {
  install.packages("nat.nblast", repos="http://cran.rstudio.com/")
}
if (!require("foreach",character.only = TRUE)) {
  install.packages("foreach", repos="http://cran.rstudio.com/")
}
if (!require("doParallel",character.only = TRUE)) {
  install.packages("doParallel", repos="http://cran.rstudio.com/")
}

library(nat.nblast)
library(nat)
library(foreach)
library(parallel)
library(doParallel)



seglength=function(ThisSeg, sum=TRUE){
  #ThisSeg is an array of x,y and z data points
  #In order to calculate the length
  #Need to find dx,dy,dz
  #Then find sqrt(dx^2+...)
  #Then sum over the path
  if(nrow(ThisSeg)==1) return(0)
  ds=diff(ThisSeg)
  edgelengths=sqrt(rowSums(ds*ds))
  if(sum) sum(edgelengths) else unname(edgelengths)
}

resample2.neuron<-function(x, stepsize, ...) {
  # extract original vertex array before resampling
  cols=c("X","Y","Z")
  if(!is.null(x$d$W)) cols=c(cols, 'W')
  # if(!is.null(x$d$Label)) cols=c(cols, 'Label')
  d=data.matrix(x$d[, cols, drop=FALSE])
  # if(!is.null(d$Label)) d$Label=as.integer(d$Label)
  if(any(is.na(d[,1:3])))
    stop("Unable to resample neurons with NA points")
  
  # fetch all segments and process each segment in turn
  sl=as.seglist(x, all = T, flatten = T)
  npoints=nrow(d)
  dl=list(d)
  sl2=list()
  count=0
  for (i in seq_along(sl)){
    s=sl[[i]]
    # interpolate this segment
    dold=d[s, , drop=FALSE]
    dnew=resample_segment2(dold, stepsize=stepsize, ...)
    if(is.null(dnew)) next
    dl[[length(dl)+1]]=dnew
    # if we've got here, we need to do something
    # add new points to the end of the swc block
    # and give them sequential point numbers
    newids=seq.int(from = npoints+1, length.out = nrow(dnew))
    npoints=npoints+nrow(dnew)
    # replace internal ids in segment so that proximal point is connected to head
    # and distal point is connected to tail
    sl[[i]]=c(s[1], newids, s[length(s)])
    
    count = count + 1
    sl2[[count]] = sl[[i]]
  }
  sl = sl2
  if (length(sl) == 0) {
    return(NULL)
  }
  
  d=do.call(rbind, dl)
  d=as.data.frame(d)
  rownames(d)=NULL
  # let's deal with the label column which was dropped - assume that always the
  # same within a segment
  head_idxs=sapply(sl, "[", 1)
  seglabels=x$d$Label[head_idxs]
  
  # in order to avoid re-ordering the segments when as.neuron.ngraph is called
  # we can renumber the raw indices in the seglist (and therefore the vertices)
  # in a strictly ascending sequence based on the seglist
  # it is *much* more efficient to compute this on a single vector rather than
  # separately on each segment in the seglist. However this does involve some
  # gymnastics 
  usl=unlist(sl)
  old_ids=unique(usl)
  # reorder vertex information to match this
  d=d[old_ids,]
  
  node_per_seg=sapply(sl, length)
  df=data.frame(id=usl, seg=rep(seq_along(sl), node_per_seg))
  df$newid=match(df$id, old_ids)
  sl=split(df$newid, df$seg)
  labels_by_seg=rep(seglabels, node_per_seg)
  # but there will be some duplicated ids (branch points) that we must remove
  d$Label=labels_by_seg[!duplicated(df$newid)]
  swc=seglist2swc(sl, d)
  as.neuron(swc)
}

# Interpolate ordered 3D points (optionally w diameter)
# NB returns NULL if unchanged (when too short or <=2 points) 
# and only returns _internal_ points, omitting the head and tail of a segment
#' @importFrom stats approx
resample_segment2<-function(d, stepsize, ...) {
  # we must have at least 2 points to resample
  if(nrow(d) < 2) return(NULL)
  
  dxyz=xyzmatrix(d)
  # we should only resample if the segment is longer than the new stepsize
  l=seglength(dxyz)
  if(l<=stepsize) return(NULL)
  
  # figure out linear position of new internal points
  internalPoints=seq(stepsize, l, by=stepsize)
  nInternalPoints=length(internalPoints)
  # if the last internal point is actually in exactly the same place 
  # as the endpoint then discard it
  if(internalPoints[nInternalPoints]==l) {
    internalPoints=internalPoints[-length(internalPoints)]
    nInternalPoints=length(internalPoints)
  }
  
  # find cumulative length stopping at each original point on the segment
  diffs=diff(dxyz)
  cumlength=c(0,cumsum(sqrt(rowSums(diffs*diffs))))
  
  # find 3D position of new internal points
  # using linear approximation on existing segments
  # make an emty object for results
  # will have same type (matrix/data.frame as input)
  dnew=matrix(nrow=nInternalPoints, ncol=ncol(d))
  colnames(dnew)=colnames(d)
  if(is.data.frame(d)){
    dnew=as.data.frame(dnew)
  }
  for(n in seq.int(ncol(dnew))) {
    dnew[,n] <- if(!all(is.finite(d[,n]))) {
      rep(NA, nInternalPoints)
    } else {
      approx(cumlength, d[,n], internalPoints, 
             method = ifelse(is.double(d[,n]), "linear", "constant"))$y
    }
  }
  dnew
}



img = NULL
swc = NULL
qdp = NULL
if (grepl("\\.swc$", infile)) {
  cat("Loading swc...\n")
  swc = read.neuron(infile)
  qdp = NULL
  step = 2.0
  repeat{
    rn = resample2.neuron(swc, stepsize = step)
    if (!is.null(rn) || step < 0.5) break
    step = step / 2.0
  }
  qdp = dotprops(rn, OmitFailures=T, k=2)
} else {
  cat("Loading images...\n")
  img = read.im3d(infile)
  qdp = dotprops(img, k=3)
}


cat("Initializing threads...\n")
th = parallel::detectCores()-1
if (th < 1) th = 1
cl <- parallel::makeCluster(th, outfile="")


tryCatch({
  
  registerDoParallel(cl)
  cat("Loading NBLAST library into each thread...\n")
  clusterCall(cl, function() suppressMessages(library(nat.nblast)))
  
  sprintf("thread num: %i", th)
  cat("Running NBLAST...\n")
  allres = neuronlist();
  allscr = numeric();
  
  for (i in 1:length(nlibs)) {
    nl <- nlibs[i]
    cat(paste(nl, "\n"))
    dp <- read.neuronlistfh(nl, localdir=dirname(nl))
    
    a <- seq(1, length(dp), by=length(dp)%/%th)
    b <- seq(length(dp)%/%th, length(dp), by=length(dp)%/%th)
    if (length(dp)%%th > 0) {
      b <- c(b, length(dp))
    }
    
    cat("calculating forward scores...\n")
    fwdscores <- foreach(aa = a, bb = b) %dopar% {
      if (aa == 1) {
        nblast(qdp, dp[aa:bb], normalised=T, UseAlpha=T, .progress='text')
      } else {
        nblast(qdp, dp[aa:bb], normalised=T, UseAlpha=T)
      }
    }
    scnames <- list()
    for (j in 1:length(fwdscores)) scnames <- c(scnames, names(fwdscores[[j]]))
    fwdscores <- unlist(fwdscores)
    names(fwdscores) <- scnames
    
    if (normalization == "mean") {
      cat("calculating reverse scores...\n")
      revscores <- foreach(aa = a, bb = b) %dopar% {
        if (aa == 1) {
          nblast(dp[aa:bb], qdp, normalised=T, UseAlpha=T, .progress='text')
        } else {
          nblast(dp[aa:bb], qdp, normalised=T, UseAlpha=T)
        }
      }
      scnames <- list()
      for (j in 1:length(revscores)) scnames <- c(scnames, names(revscores[[j]]))
      revscores <- unlist(revscores)
      names(revscores) <- scnames
      
      scores <- (fwdscores + revscores) / 2
    } else {
      scores <- fwdscores
    }
    
    cat("sorting scores...\n")
    scores <- sort(scores, dec=T)
    
    if (length(scores) <= resultnum) {
      #results <- as.neuronlist(dp[names(scores)])
      slist <- scores
    } else {
      #results <- as.neuronlist(dp[names(scores)[1:resultnum]])
      slist <- scores[1:resultnum]
    }
    
    cat("setting names...\n")
    #for (j in 1:length(results)) {
    #  names(results)[j] <- paste(names(results[j]), as.character(i-1), sep=",")
    #}
    for (j in 1:length(slist)) {
      names(slist)[j] <- paste(names(slist[j]), as.character(i-1), sep=",")
    }
    
    cat("combining lists...\n")
    #allres <- c(allres, results)
    allscr <- c(allscr, slist)
    
    rm(dp)
    gc()
  }
  
  allscr = sort(allscr, dec=T)
  if (length(allscr) <= resultnum) {
    #results = allres[names(allscr)]
    slist = allscr
  } else {
    #results = allres[names(allscr)[1:resultnum]]
    slist = allscr[1:resultnum]
  }
  
  cat("Writing results...\n")
  swczipname = paste(outfname, ".zip", sep="")
  rlistname  = paste(outfname, ".txt", sep="")
  zprojname  = paste(outfname, ".png", sep="")
  
  f = file(file.path(outputdir,rlistname))
  writeLines(c(dbnames, nlibpath), con=f)
  write.table(format(slist, digits=8), append=T, file.path(outputdir,rlistname), sep=",", quote=F, col.names=F, row.names=T)
  #n = names(results)
  #n = gsub(",", " db=", n)
  #write.neurons(results, dir=file.path(outputdir,swczipname), files=n, format='swc', Force=T)
  
  if (!is.null(img)) {
    zproj = projection(img, projfun=max)
    size = dim(zproj)
    png(file.path(outputdir,zprojname), width=size[1], height=size[2])
    par(plt=c(0,1,0,1))
    image(zproj, col = grey(seq(0, 1, length = 256)))
    dev.off()
  } else {
    png(file.path(outputdir,zprojname), width=256, height=256, bg='black')
    plot.new()
    dev.off()
  }
  
}, 
error = function(e) {
  print(e)
  Sys.sleep(5)
  stopCluster(cl)
})

stopCluster(cl)

cat("Done\n")

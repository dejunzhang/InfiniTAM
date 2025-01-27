
#define _USE_MATH_DEFINES
#include <math.h>
#include "Scene.h" // defines: #include "HashMap.h"
#include <stdio.h>

template<typename T>
struct Z3Hasher {
    typedef T KeyType;
    static const uint BUCKET_NUM = 0x1000; // Number of Hash Bucket, must be 2^n (otherwise we have to use % instead of & below)

    static GPU_ONLY uint hash(const T& blockPos) {
        return (((uint)blockPos.x * 73856093u) ^ ((uint)blockPos.y * 19349669u) ^ ((uint)blockPos.z * 83492791u))
            &
            (uint)(BUCKET_NUM - 1);
    }
};


KERNEL get(HashMap<Z3Hasher<Vector3s>>* myHash, Vector3s q, int* o) {
    *o = myHash->getSequenceNumber(q);
}

KERNEL alloc(HashMap<Z3Hasher<Vector3s>>* myHash) {
    int p = blockDim.x * blockIdx.x + threadIdx.x;
    myHash->requestAllocation(p);
}

#include <vector>
using namespace std;
KERNEL assertfalse() {
    assert(false);
}


void testZ3Hasher() {
    //assertfalse << <1, 1 >> >();
    //assert(false);
    // insert a lot of points into a large hash just for fun
    HashMap<Z3Hasher<Vector3s>>* myHash = new HashMap<Z3Hasher<Vector3s>>(0x2000);

    int n = 1000;
    LAUNCH_KERNEL(alloc,n, 1 ,myHash);

    myHash->performAllocations();
    puts("after alloc");
    // should be some permutation of 1:n
    vector<bool> found; found.resize(n + 1);
    int* p; cudaMallocManaged(&p, sizeof(int));
    for (int i = 0; i < n; i++) {
        LAUNCH_KERNEL(get, 
            1, 1, 
            myHash, Vector3s(i, i, i), p);
        cudaSafeCall(cudaDeviceSynchronize()); // to read managed p
        printf("Vector3s(%i,%i,%i) -> %d\n", i, i, i, *p);

        assert(!found[*p]);
        found[*p] = 1;
    }
}

// n hasher test suite
// trivial hash function n -> n
struct NHasher{
    typedef int KeyType;
    static const uint BUCKET_NUM = 1;
    static GPU_ONLY uint hash(const int& n) {
        return n % BUCKET_NUM;//& (BUCKET_NUM-1);
    }
};

KERNEL get(HashMap<NHasher>* myHash, int p, int* o) {
    *o = myHash->getSequenceNumber(p);
}

KERNEL alloc(HashMap<NHasher>* myHash, int p, int* o) {
    myHash->requestAllocation(p);
}

void testNHasher() {
    int n = NHasher::BUCKET_NUM;
    auto myHash = new HashMap<NHasher>(1 + 1); // space for BUCKET_NUM entries only, and 1 collision handling entry

    int* p; cudaMallocManaged(&p, sizeof(int));

    for (int i = 0; i < n; i++) {

        LAUNCH_KERNEL(alloc,
            1, 1,
            myHash, i, p);
    }
    myHash->performAllocations();

    // an additional alloc at another key not previously seen (e.g. BUCKET_NUM) 
    alloc << <1, 1 >> >(myHash, NHasher::BUCKET_NUM, p);
    myHash->performAllocations();

    // an additional alloc at another key not previously seen (e.g. BUCKET_NUM + 1) makes it crash cuz no excess list
    //alloc << <1, 1 >> >(myHash, NHasher::BUCKET_NUM + 1, p);
    myHash->performAllocations(); // performAllocations is always fine to call when no extra allocations where made

    puts("after alloc");
    // should be some permutation of 1:BUCKET_NUM
    bool found[NHasher::BUCKET_NUM + 1] = {0};
    for (int i = 0; i < n; i++) {
        get << <1, 1 >> >(myHash, i, p);
        cudaDeviceSynchronize();
        printf("%i -> %d\n", i, *p);
        assert(!found[*p]);
        //assert(*p != i+1); // numbers are very unlikely to be in order -- nah it happens
        found[*p] = 1;
    }

}

// zero hasher test suite
// trivial hash function with one bucket.
// This will allow the allocation of only one block at a time
// and all blocks will be in the same list.
// The numbers will be in order.
struct ZeroHasher{
    typedef int KeyType;
    static const uint BUCKET_NUM = 0x1;
    static GPU_ONLY uint hash(const int&) { return 0; }
};

KERNEL get(HashMap<ZeroHasher>* myHash, int p, int* o) {
    *o = myHash->getSequenceNumber(p);
}

KERNEL alloc(HashMap<ZeroHasher>* myHash, int p, int* o) {
    myHash->requestAllocation(p);
}

void testZeroHasher() {
    int n = 10;
    auto myHash = new HashMap<ZeroHasher>(n); // space for BUCKET_NUM(1) + excessnum(n-1) = n entries
    assert(myHash->getLowestFreeSequenceNumber() == 1);
    int* p; cudaMallocManaged(&p, sizeof(int));

    const int extra = 0; // doing one more will crash it at
    // Assertion `excessListEntry >= 1 && excessListEntry < EXCESS_NUM` failed.

    // Keep requesting allocation until all have been granted
    for (int j = 0; j < n + extra; j++) { // request & perform alloc cycle
        for (int i = 0; i < n + extra
            ; i++) {
            alloc << <1, 1 >> >(myHash, i, p); // only one of these allocations will get through at a time
        }
        myHash->performAllocations();

        puts("after alloc");
        for (int i = 0; i < n; i++) {
            get << <1, 1 >> >(myHash, i, p);
            cudaDeviceSynchronize();
            printf("%i -> %d\n", i, *p);
            // expected result
            assert(i <= j ? *p == i + 1 : *p == 0);
        }
    }

    assert(myHash->getLowestFreeSequenceNumber() != 1);
}
#include "Cholesky.h"
using namespace ORUtils;
void testCholesky() {
    float m[] = {
        1, 0,
        0, 1
    };
    float b[] = {1, 2};
    float r[2];
    Cholesky::solve(m, 2, b, r);
    assert(r[0] == b[0] && r[1] == b[1]);

}

KERNEL addSceneVB(Scene* scene) {
    assert(scene);
    scene->requestVoxelBlockAllocation(VoxelBlockPos(0, 0, 0));
    scene->requestVoxelBlockAllocation(VoxelBlockPos(1,2,3));
}

GPU_ONLY void allExist(Scene* scene, Vector3i base) {
    for (int i = 0; i < SDF_BLOCK_SIZE; i++)
        for (int j = 0; j < SDF_BLOCK_SIZE; j++)
            for (int k = 0; k < SDF_BLOCK_SIZE; k++) {
                ITMVoxel* v = scene->getVoxel(base + Vector3i(i, j, k));
                assert(v != NULL);
            }
}
KERNEL findSceneVoxel(Scene* scene) {
    allExist(scene, Vector3i(0,0,0));
    allExist(scene, Vector3i(SDF_BLOCK_SIZE, 2*SDF_BLOCK_SIZE, 3*SDF_BLOCK_SIZE));

    assert(scene->getVoxel(Vector3i(-1, 0, 0)) == NULL);
}

KERNEL checkS(Scene* scene) {
    assert(Scene::getCurrentScene() == scene);
}

struct WriteEach {
    doForEachAllocatedVoxel_process() {
        v->setSDF((
            localPos.x + 
            localPos.y * SDF_BLOCK_SIZE + 
            localPos.z * SDF_BLOCK_SIZE * SDF_BLOCK_SIZE
            )/1024.f);
    }
};

__managed__ int counter = 0;
__managed__ bool visited[SDF_BLOCK_SIZE][SDF_BLOCK_SIZE][SDF_BLOCK_SIZE] = {0};
struct DoForEach {
    doForEachAllocatedVoxel_process() {
        assert(localPos.x >= 0 && localPos.y >= 0 && localPos.z >= 0);
        assert(localPos.x  < SDF_BLOCK_SIZE && localPos.y < SDF_BLOCK_SIZE && localPos.z < SDF_BLOCK_SIZE);

        assert(vb);
        assert(vb->pos == VoxelBlockPos(0, 0, 0) ||
            vb->pos == VoxelBlockPos(1,2,3)); 

        visited[localPos.x][localPos.y][localPos.z] = 1;

        printf("%f .. %f\n", v->getSDF(),
            (
            localPos.x +
            localPos.y * SDF_BLOCK_SIZE +
            localPos.z * SDF_BLOCK_SIZE * SDF_BLOCK_SIZE
            ) / 1024.f);
        assert(abs(
            v->getSDF() -
            (
            localPos.x +
            localPos.y * SDF_BLOCK_SIZE +
            localPos.z * SDF_BLOCK_SIZE * SDF_BLOCK_SIZE
            ) / 1024.f) < 0.001 // not perfectly accurate
            );
        atomicAdd(&counter, 1);
    }
};
struct DoForEachBlock {
    static GPU_ONLY void process(ITMVoxelBlock* vb) {
        assert(vb);
        assert(vb->pos == VoxelBlockPos(0, 0, 0) ||
            vb->pos == VoxelBlockPos(1, 2, 3));
        atomicAdd(&counter, 1);
    }
};

KERNEL modifyS() {
    Scene::getCurrentSceneVoxel(Vector3i(0, 0, 1))->setSDF(1.0);
}

KERNEL checkModifyS() {
    assert(Scene::getCurrentSceneVoxel(Vector3i(0, 0, 1))->getSDF() == 1.0);
}

void testScene() {
    assert(Scene::getCurrentScene() == 0);

    Scene* s = new Scene(); 
    LAUNCH_KERNEL(addSceneVB, 1, 1, s);
    s->performAllocations();
    LAUNCH_KERNEL(findSceneVoxel, 1, 1, s);

    // current scene starts out at 0
    LAUNCH_KERNEL(checkS, 1, 1, 0);

    // change current scene
    {
        LAUNCH_KERNEL(checkS, 1, 1, 0); // still 0 before scope begins

        CURRENT_SCENE_SCOPE(s);
        LAUNCH_KERNEL(checkS, 1, 1, s);
        // Nest
        {
            CURRENT_SCENE_SCOPE(0);
            LAUNCH_KERNEL(checkS, 1, 1, 0);
        }
        LAUNCH_KERNEL(checkS, 1, 1, s);
    }
    LAUNCH_KERNEL(checkS, 1, 1, 0); // 0 again

    // modify current scene
    {
        CURRENT_SCENE_SCOPE(s);
        LAUNCH_KERNEL(modifyS, 1, 1);
        LAUNCH_KERNEL(checkModifyS, 1, 1);
    }

    // do for each

    s->doForEachAllocatedVoxel<WriteEach>();

    counter = 0;
    for (int x = 0; x < SDF_BLOCK_SIZE; x++)
        for (int y = 0; y < SDF_BLOCK_SIZE; y++)
            for (int z = 0; z < SDF_BLOCK_SIZE; z++)
                assert(!visited[x][y][z]);
    s->doForEachAllocatedVoxel<DoForEach>();
    cudaDeviceSynchronize();
    assert(counter == 2 * SDF_BLOCK_SIZE3);
    for (int x = 0; x < SDF_BLOCK_SIZE; x++)
        for (int y = 0; y < SDF_BLOCK_SIZE; y++)
            for (int z = 0; z < SDF_BLOCK_SIZE; z++)
                assert(visited[x][y][z]);

    counter = 0;
    s->doForEachAllocatedVoxelBlock<DoForEachBlock>();
    assert(counter == 2);

    delete s;
}

#define W 5
#define H 7
#include "ITMPixelUtils.h"
struct DoForEachPixel {
    forEachPixelNoImage_process() {
        assert(x >= 0 && x < W);
        assert(y >= 0 && y < H);
        atomicAdd(&counter, 1);
    }
};

void testForEachPixelNoImage() {
    counter = 0;
    forEachPixelNoImage<DoForEachPixel>(Vector2i(W, H));
    cudaDeviceSynchronize();
    assert(counter == W * H);
}

#include "FileUtils.h"
#include "itmcalibio.h"
#include "itmlibdefines.h"
#include "itmview.h"
#include "ITMMath.h"

#include <fstream>
#include <vector>
#include <algorithm>
using namespace std;


extern void FuseView_pre(
    const ITMView * const view,
    Matrix4f M_d
    );

void approxEqual(float a, float b, const float eps = 0.00001) {
    assert(abs(a - b) < eps);
}


void approxEqual(Matrix4f a, Matrix4f b, const float eps = 0.00001) {
    for (int i = 0; i < 4 * 4; i++)
        approxEqual(a.m[i], b.m[i], eps);
}

void approxEqual(Matrix3f a, Matrix3f b, const float eps = 0.00001) {
    for (int i = 0; i < 3 * 3; i++)
        approxEqual(a.m[i], b.m[i], eps);
}

void testAllocRequests(Matrix4f M_d, 
    const char* expectedRequestsFilename, 
    const char* missedExpectedRequestsFile,
    const char* possibleExtra = 0) {
    // [[
    std::vector<VoxelBlockPos> allExpectedRequests;
    {
        ifstream expectedRequests(expectedRequestsFilename);
        assert(expectedRequests.is_open());
        while (1) {
            VoxelBlockPos expectedBlockCoord;
            expectedRequests >> expectedBlockCoord.x >> expectedBlockCoord.y >> expectedBlockCoord.z;
            if (expectedRequests.fail()) break;
            allExpectedRequests.push_back(expectedBlockCoord);
        }
    }

    std::vector<VoxelBlockPos> allMissedExpectedRequests;
    {
        ifstream expectedRequests(missedExpectedRequestsFile);
        assert(expectedRequests.is_open());
        while (1) {
            VoxelBlockPos expectedBlockCoord;
            expectedRequests >> expectedBlockCoord.x >> expectedBlockCoord.y >> expectedBlockCoord.z;
            if (expectedRequests.fail()) break;
            allMissedExpectedRequests.push_back(expectedBlockCoord);
        }
    }

    // Some requests might be lost entirely sometimes
    std::vector<VoxelBlockPos> extra;
    if (possibleExtra)
    {
        ifstream expectedRequests(possibleExtra);
        assert(expectedRequests.is_open());
        while (1) {
            VoxelBlockPos expectedBlockCoord;
            expectedRequests >> expectedBlockCoord.x >> expectedBlockCoord.y >> expectedBlockCoord.z;
            if (expectedRequests.fail()) break;
            extra.push_back(expectedBlockCoord);
        }
    }

    // ]]
    ITMUChar4Image rgb(Vector2i(1,1));
    png::ReadImageFromFile(&rgb, "Tests\\TestAllocRequests\\color1.png");
    ITMShortImage depth(Vector2i(1, 1));
    png::ReadImageFromFile(&depth, "Tests\\TestAllocRequests\\depth1.png");
    ITMRGBDCalib calib;
    readRGBDCalib("Tests\\TestAllocRequests\\calib.txt", calib);

    ITMView* view = new ITMView(&calib);
    ITMView::depthConversionType = "ConvertDisparityToDepth";
    view->Update(&rgb, &depth);
    cudaDeviceSynchronize();

    assert(view->depth->noDims == Vector2i(640, 480));
    assert(view->rgb->noDims == Vector2i(640, 480));
    assert(view->calib->intrinsics_d.getInverseProjParams() ==
        Vector4f(0.00174304086	,
        0.00174096529	,
        346.471008	,
        249.031006	));

    Scene* scene = new Scene();
    CURRENT_SCENE_SCOPE(scene);

    // test FuseView_pre
    FuseView_pre(
        view, M_d
        );

    cudaDeviceSynchronize();

    // test content of requests "allocate planned"
    uchar *entriesAllocType = (uchar *)malloc(SDF_GLOBAL_BLOCK_NUM);
    Vector3s *blockCoords = (Vector3s *)malloc(SDF_GLOBAL_BLOCK_NUM * sizeof(Vector3s));

    cudaMemcpy(entriesAllocType,
        Scene::getCurrentScene()->voxelBlockHash->needsAllocation,
        SDF_GLOBAL_BLOCK_NUM,
        cudaMemcpyDeviceToHost);

    cudaMemcpy(blockCoords,
        Scene::getCurrentScene()->voxelBlockHash->naKey,
        SDF_GLOBAL_BLOCK_NUM * sizeof(VoxelBlockPos),
        cudaMemcpyDeviceToHost);
    {
        ifstream expectedRequests(expectedRequestsFilename);
        assert(expectedRequests.is_open());
        VoxelBlockPos expectedBlockCoord;
        bool read = true;
        for (int targetIdx = 0; targetIdx < SDF_GLOBAL_BLOCK_NUM; targetIdx++) {
            if (entriesAllocType[targetIdx] == 0) continue;
            
            if (read)
                expectedRequests >> expectedBlockCoord.x >> expectedBlockCoord.y >> expectedBlockCoord.z;
            read = true;
            assert(!expectedRequests.fail());

            printf("expecting %d %d %d got %d %d %d\n", 
                xyz(expectedBlockCoord),
                xyz(blockCoords[targetIdx])
                );

            if (expectedBlockCoord != blockCoords[targetIdx]) {
                // If the expectedBlockCoord is not in this file, it must be in the missed requests - 
                // it is not deterministic which blocks will be allocated first and which on the second run
                auto i = find(allMissedExpectedRequests.begin(), allMissedExpectedRequests.end(),
                    blockCoords[targetIdx]);
                if (i == allMissedExpectedRequests.end()) {
                    auto i = find(
                        extra.begin(),
                        extra.end(),
                        blockCoords[targetIdx]);
                    read = false;
                    assert(i != extra.end());
                }
            }

            continue;
        }
        // Must have seen all requests
        int _;
        expectedRequests >> _;
        assert(expectedRequests.fail());
    }

    // do allocations
    Scene::performCurrentSceneAllocations();

    cudaDeviceSynchronize();
    // --- again!
    // test FuseView_pre
    FuseView_pre(
        view, M_d
        );

    cudaDeviceSynchronize();

    // test content of requests "allocate planned"
    cudaMemcpy(entriesAllocType,
        Scene::getCurrentScene()->voxelBlockHash->needsAllocation,
        SDF_GLOBAL_BLOCK_NUM,
        cudaMemcpyDeviceToHost);

    cudaMemcpy(blockCoords,
        Scene::getCurrentScene()->voxelBlockHash->naKey,
        SDF_GLOBAL_BLOCK_NUM * sizeof(VoxelBlockPos),
        cudaMemcpyDeviceToHost);

    {
        ifstream expectedRequests(missedExpectedRequestsFile);
        assert(expectedRequests.is_open());
        for (int targetIdx = 0; targetIdx < SDF_GLOBAL_BLOCK_NUM; targetIdx++) {
            if (entriesAllocType[targetIdx] == 0) continue;
            VoxelBlockPos expectedBlockCoord;
            expectedRequests >> expectedBlockCoord.x >> expectedBlockCoord.y >> expectedBlockCoord.z;

            if (expectedBlockCoord != blockCoords[targetIdx]) {
                // If the expectedBlockCoord is not in this file, it must be in the missed requests - 
                // it is not deterministic which blocks will be allocated first and which on the second run
                auto i = find(allExpectedRequests.begin(), allExpectedRequests.end(),
                    blockCoords[targetIdx]);
                if (i == allExpectedRequests.end()) {
                    auto i = find(
                        extra.begin(),
                        extra.end(),
                        blockCoords[targetIdx]);
                    assert(i != extra.end());
                }
            }

        }
        // Must have seen all requests
        int _;
        expectedRequests >> _;
        assert(expectedRequests.fail());
    }

    delete scene;
    delete view;
}
/// Must exist on cpu
template<typename T>
static bool checkImageSame(Image<T>* a_, Image<T>* b_) {
    T* a = a_->GetData(MEMORYDEVICE_CPU);
    T* b = b_->GetData(MEMORYDEVICE_CPU);
#define failifnot(x) if (!(x)) return false;
    failifnot(a_->dataSize == b_->dataSize);
    failifnot(a_->noDims == b_->noDims);
    int s = a_->dataSize;
    while (s--) {
        if (*a != *b) {
            failifnot(false);
        }
        a++;
        b++;
    }
    return true;
}

template<>
static bool checkImageSame(Image<Vector4u>* a_, Image<Vector4u>* b_) {
    Vector4u* a = a_->GetData(MEMORYDEVICE_CPU);
    Vector4u* b = b_->GetData(MEMORYDEVICE_CPU);
#define failifnot(x) if (!(x)) return false;
    failifnot(a_->dataSize == b_->dataSize);
    failifnot(a_->noDims == b_->noDims);
    int s = a_->dataSize;
    while (s--) {
        if (*a != *b) {
            png::SaveImageToFile(a_, "checkImageSame_a.png");
            png::SaveImageToFile(b_, "checkImageSame_b.png");
            failifnot(false);
        }
        a++;
        b++;
    }
    return true;
}

template<>
static bool checkImageSame(Image<short>* a_, Image<short>* b_) {
    short* a = a_->GetData(MEMORYDEVICE_CPU);
    short* b = b_->GetData(MEMORYDEVICE_CPU);
#define failifnot(x) if (!(x)) return false;
    failifnot(a_->dataSize == b_->dataSize);
    failifnot(a_->noDims == b_->noDims);
    int s = a_->dataSize;
    while (s--) {
        if (*a != *b) {
            png::SaveImageToFile(a_, "checkImageSame_a.png");
            png::SaveImageToFile(b_, "checkImageSame_b.png");
            failifnot(false);
        }
        a++;
        b++;
    }
    return true;
}

/// Must exist on cpu
template<typename T>
static void assertImageSame(Image<T>* a_, Image<T>* b_) {
    assert(checkImageSame(a_,b_));
}

ITMUChar4Image* image(std::string fn) {
    ITMUChar4Image* i = new ITMUChar4Image(Vector2i(1, 1));
    assert(png::ReadImageFromFile(i, fn));
    return i;

}
void testImageSame() {
    auto i = image("Tests\\TestAllocRequests\\color1.png");
    assertImageSame(i,i);
    delete i;
}

#include "itmlib.h"
#include "ITMVisualisationEngine.h"

ITMUChar4Image* renderNow(Vector2i imgSize, ITMPose* pose) {
    ITMRenderState* renderState_freeview = new ITMRenderState(imgSize);
    auto render = new ITMUChar4Image(imgSize);

    RenderImage(
        pose, new ITMIntrinsics(),
        renderState_freeview,
        render, "renderGrey");
    delete renderState_freeview;
    return render;
}

void renderExpecting(const char* fn, ITMPose* pose = new ITMPose()) {
    auto expect = image(fn);
    auto render = renderNow(expect->noDims, pose);
    assertImageSame(expect, render);
    delete expect;
    delete render;
}

#define make(scene) Scene* scene = new Scene(); CURRENT_SCENE_SCOPE(scene);

void testRenderBlack() {
    make(scene);
    renderExpecting("Tests\\TestRender\\black.png");
    delete scene;
}


static KERNEL buildBlockRequests(Vector3i offset) {
    Scene::requestCurrentSceneVoxelBlockAllocation(
        VoxelBlockPos(
        offset.x + blockIdx.x,
        offset.y + blockIdx.y,
        offset.z + blockIdx.z));
}
static KERNEL countAllocatedBlocks(Vector3i offset) {
    if (Scene::getCurrentSceneVoxel(
        VoxelBlockPos(
        offset.x + blockIdx.x,
        offset.y + blockIdx.y,
        offset.z + blockIdx.z).toInt() * SDF_BLOCK_SIZE
        ))
        atomicAdd(&counter, 1);
}

// assumes buildWallRequests has been executed
// followed by perform allocations
// builds a solid wall, i.e.
// an trunctated sdf reaching 0 at 
// z == (SDF_BLOCK_SIZE / 2)*voxelSize
// and negative at bigger z.
struct BuildWall {
    static GPU_ONLY void process(const ITMVoxelBlock* vb, ITMVoxel* v, const Vector3i localPos) {
        assert(v);

        float z = (threadIdx.z) * voxelSize;
        float eta = (SDF_BLOCK_SIZE / 2)*voxelSize - z;
        v->setSDF(MAX(MIN(1.0f, eta / mu), -1.f));
    }
};
void buildWallScene() {
    // Build wall scene
    buildBlockRequests << <dim3(10, 10, 1), 1 >> >(Vector3i(0,0,0));
    cudaDeviceSynchronize();
    Scene::performCurrentSceneAllocations();
    cudaDeviceSynchronize();
    Scene::getCurrentScene()->doForEachAllocatedVoxel<BuildWall>();
}


static KERNEL buildSphereRequests() {
    Scene::requestCurrentSceneVoxelBlockAllocation(
        VoxelBlockPos(blockIdx.x,
        blockIdx.y,
        blockIdx.z));
}

static __managed__ float radiusInWorldCoordinates;
struct BuildSphere {
    static GPU_ONLY void process(const ITMVoxelBlock* vb, ITMVoxel* v, const Vector3i localPos) {
        assert(v);
        assert(radiusInWorldCoordinates > 0);

        Vector3f voxelGlobalPos = (vb->getPos().toFloat() * SDF_BLOCK_SIZE + localPos.toFloat()) * voxelSize;

        // Compute distance to origin
        const float distanceToOrigin = length(voxelGlobalPos);
        // signed distance to radiusInWorldCoordinates, positive when bigger
        const float dist = distanceToOrigin - radiusInWorldCoordinates;
        
        // Truncate and convert to -1..1 for band of size mu
        const float eta = dist;
        v->setSDF(MAX(MIN(1.0f, eta / mu), -1.f));
    }
};
void buildSphereScene(const float radiusInWorldCoordinates) {
    assert(radiusInWorldCoordinates > 0);
    ::radiusInWorldCoordinates = radiusInWorldCoordinates;
    const float diameterInWorldCoordinates = radiusInWorldCoordinates * 2;
    int offseti = -ceil(radiusInWorldCoordinates / voxelBlockSize) - 1; // -1 for extra space
    assert(offseti < 0);

    Vector3i offset(offseti, offseti, offseti);
    int counti = ceil(diameterInWorldCoordinates / voxelBlockSize) + 2; // + 2 for extra space
    assert(counti > 0);
    dim3 count(counti, counti, counti);
    assert(offseti + count.x == -offseti);

    // repeat allocation a few times to avoid holes
    do {
        buildBlockRequests << <count, 1 >> >(offset);
        cudaDeviceSynchronize();
        Scene::performCurrentSceneAllocations();
        cudaDeviceSynchronize();
        counter = 0;
        countAllocatedBlocks << <count, 1 >> >(offset);
        cudaDeviceSynchronize();
    } while (counter != counti*counti*counti);

    Scene::getCurrentScene()->doForEachAllocatedVoxel<BuildSphere>();
}


void testRenderWall() {
    make(scene);

    renderExpecting("Tests\\TestRender\\black.png");

    buildWallScene();

    renderExpecting("Tests\\TestRender\\black.png");

    // move everything away a bit so we can see the wall
    auto pose = new ITMPose();
    pose->SetT(Vector3f(0, 0, voxelSize * 100)); 

    renderExpecting("Tests\\TestRender\\wall.png", pose);
    renderExpecting("Tests\\TestRender\\wall.png", pose); // unchanged
    pose->SetT(Vector3f(0, 0, 0)); // nothing again
    renderExpecting("Tests\\TestRender\\black.png");
    delete scene;
}

void testAllocRequests() {

    // With the alignment generated by the (buggy?) original Track Camera on the first frame,
    // no conflicts occur
    Matrix4f M_d;
    M_d.m00 = 0.848863006;
    M_d.m01 = 0.441635638;
    M_d.m02 = -0.290498704;
    M_d.m03 = 0.000000000;
    M_d.m10 = -0.290498704;
    M_d.m11 = 0.848863065;
    M_d.m12 = 0.441635549;
    M_d.m13 = 0.000000000;
    M_d.m20 = 0.441635638;
    M_d.m21 = -0.290498614;
    M_d.m22 = 0.848863065;
    M_d.m23 = 0.000000000;
    M_d.m30 = -0.144862041;
    M_d.m31 = -0.144861951;
    M_d.m32 = -0.144861966;
    M_d.m33 = 1.00000000;

    Matrix4f invM_d; M_d.inv(invM_d);
    approxEqual(invM_d.m00, 0.848863125); // exactly equal should do
    approxEqual(invM_d.m01, -0.290498734);
    approxEqual(invM_d.m02, 0.441635668);
    approxEqual(invM_d.m03, 0.000000000);
    approxEqual(invM_d.m10, 0.441635668);
    approxEqual(invM_d.m11, 0.848863184);
    approxEqual(invM_d.m12, -0.290498614);
    approxEqual(invM_d.m13, 0.000000000);
    approxEqual(invM_d.m20, -0.290498734);
    approxEqual(invM_d.m21, 0.441635609);
    approxEqual(invM_d.m22, 0.848863184);
    approxEqual(invM_d.m23, 0.000000000);
    approxEqual(invM_d.m30, 0.144862026);
    approxEqual(invM_d.m31, 0.144861937);
    approxEqual(invM_d.m32, 0.144862041);
    approxEqual(invM_d.m33, 1.00000012);
    testAllocRequests(M_d, "Tests\\TestAllocRequests\\expectedRequests.txt"
        , "Tests\\TestAllocRequests\\expectedMissedRequests.txt");

}

void testAllocRequests2() {

    // With identity matrix, we have some conflicts that are only resolved on a second allocation pass
    testAllocRequests(Matrix4f(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1), "Tests\\TestAllocRequests\\expectedRequests2.txt"
        , "Tests\\TestAllocRequests\\expectedMissedRequests2.txt"
        , "Tests\\TestAllocRequests\\possibleExtraRequests2.txt"
        );
}


void testDump() {
    auto i = image("Tests\\TestAllocRequests\\color1.png");
    assert(dump::SaveImageToFile(i, "Tests\\TestAllocRequests\\color1.dump"));
    auto j = new ITMUChar4Image(Vector2i(1, 1));
    assert(dump::ReadImageFromFile(j, "Tests\\TestAllocRequests\\color1.dump"));
    assertImageSame(i, j);
    delete i;
    delete j;

    Vector2i a(rand(), rand());
    assert(dump::SavePODToFile(&a, "dump"));
    Vector2i b;
    assert(a != b);
    assert(!dump::ReadPODFromFile(&b, "nonexistent_dump"));
    assert(dump::ReadPODFromFile(&b, "dump"));
    assert(a == b);
}

void testTracker() {
    auto trackingState = dump::load<ITMTrackingState>("Tests/Tracker/trackingState");
    auto view = dump::load<ITMView>("Tests/Tracker/view");

    TrackCamera(trackingState, view); // affects relative orientation of blocks to camera because the pose is not left unchanged even on the first try (why? on what basis is the camera moved?
    // store output  
    auto expectedPose = dump::load<ITMPose>("Tests/Tracker/out.pose_d");

    approxEqual(expectedPose->GetM(), trackingState->pose_d->GetM(), 0.003f);
    return;
}

#include "engine/imagesourceengine.h"
void testImageSource() {
    ImageFileReader r(
        "Tests\\TestAllocRequests\\calib.txt", 
        "Tests\\TestAllocRequests\\color%d.png",
        "Tests\\TestAllocRequests\\depth%d.png", 1);
    auto rgb = new ITMUChar4Image();
    auto depth = new ITMShortImage();
    r.nextImages(rgb, depth);
    assert(rgb->noDims == Vector2i(640, 480));
    assert(depth->noDims == Vector2i(640, 480));

    assert(r.calib.intrinsics_d.projectionParamsSimple.all != Vector4f(0, 0, 0, 0));

    assertImageSame(rgb, image("Tests\\TestAllocRequests\\color1.png"));

    auto odepth = new ITMShortImage();
    png::ReadImageFromFile(odepth, "Tests\\TestAllocRequests\\depth1.png");
    assertImageSame(depth, odepth);

    delete odepth;
    delete rgb;
    delete depth;
}

__managed__ int* data;
KERNEL set_data() {
    data[1] = 42;
}
KERNEL check_data() {
    assert(data[1] == 42);
}
void testMemblock() {   
    cudaDeviceSynchronize();
    auto mem = new MemoryBlock<int>(10);
    assert(mem->dataSize == 10);
    assert(!mem->dirtyCPU);
    assert(!mem->dirtyGPU);

    mem->GetData(MEMORYDEVICE_CPU);
    assert(mem->dirtyCPU);
    assert(!mem->dirtyGPU);

    mem->GetData(MEMORYDEVICE_CUDA);
    assert(!mem->dirtyCPU);
    assert(mem->dirtyGPU);

    mem->Clear(0);
    assert(!mem->dirtyCPU);
    assert(!mem->dirtyGPU);

    auto const* const cmem = mem;
    cmem->GetData(MEMORYDEVICE_CPU);
    assert(!mem->dirtyCPU);
    assert(!mem->dirtyGPU);
    cmem->GetData(MEMORYDEVICE_CUDA);
    assert(!mem->dirtyCPU);
    assert(!mem->dirtyGPU);

    mem->GetData()[1] = 42;
    assert(mem->dirtyCPU);
    assert(!mem->dirtyGPU);
    data = mem->GetData(MEMORYDEVICE_CUDA);
    assert(!mem->dirtyCPU);
    assert(mem->dirtyGPU);
    check_data << <1, 1 >> >();
    cudaDeviceSynchronize();

    mem->Clear(0);

    // NOTE wrongly assumes that everything is still clean because we reused the pointer instead of claiming it again
    // consequently, memory will not be equal.
    set_data << <1, 1 >> >();
    cudaDeviceSynchronize();
    assert(!mem->dirtyCPU);
    assert(!mem->dirtyGPU);
    check_data << <1, 1 >> >();
    cudaDeviceSynchronize();
    assert(mem->GetData()[1] == 0);

    // re-requesting fixes the problem and syncs the buffers again
    mem->Clear(0);
    data = mem->GetData(MEMORYDEVICE_CUDA);
    set_data << <1, 1 >> >();
    check_data << <1, 1 >> >();
    cudaDeviceSynchronize();
    assert(mem->GetData()[1] == 42);
}

#include <memory>
using namespace std;
void testMainEngine() {
    auto_ptr<ITMMainEngine> me(new ITMMainEngine(new ITMRGBDCalib()));

    ITMPose* pose = new ITMPose();
    pose->SetT(Vector3f(0, 0, voxelSize * 100));
    auto imgSize = Vector2i(640, 480);
    auto render = new ITMUChar4Image(imgSize);

    me->GetImage(render, pose, new ITMIntrinsics(), "renderGrey");
    assertImageSame(image("Tests\\TestRender\\black.png"), render);

    me->GetImage(render, pose, new ITMIntrinsics(), "renderGrey");
    assertImageSame(image("Tests\\TestRender\\black.png"), render);

    CURRENT_SCENE_SCOPE(me->scene);
    buildWallScene();

    me->GetImage(render, pose, new ITMIntrinsics(), "renderGrey");
    assertImageSame(image("Tests\\TestRender\\wall.png"), render);

    pose->SetT(Vector3f(0, 0, 0));

    me->GetImage(render, pose, new ITMIntrinsics(), "renderGrey");
    assertImageSame(image("Tests\\TestRender\\black.png"), render);

    delete pose;
    delete render;
}

#include "itmview.h"
void testMainEngineProcessFrame() {
    auto expectedRaycastResult = new Image<Vector4f>();
    auto expectedRaycastResult2 = new Image<Vector4f>();

    auto imageSource = new ImageFileReader(
        "Tests\\TestFountain\\calib.txt",
        "Tests\\TestFountain\\color%i.png",
        "Tests\\TestFountain\\depth%i.png",
        1);
    ITMView::depthConversionType = "ScaleAndValidateDepth";
    auto rgb = new ITMUChar4Image();
    auto depth = new ITMShortImage();
    assert(imageSource->currentFrameNo == 1);
    imageSource->nextImages(rgb, depth);
    assert(rgb->noDims.area() > 1);
    assert(depth->noDims.area() > 1);
    assert(imageSource->currentFrameNo == 2);
    auto mainEngine = new ITMMainEngine(&imageSource->calib);
    assert(!mainEngine->renderState_freeview);
    assert(!mainEngine->renderState_live);

    mainEngine->ProcessFrame(rgb, depth);
    assert(mainEngine->GetView()->depth->noDims == depth->noDims);
    assert(mainEngine->GetView()->rgb->noDims == rgb->noDims);
    assert(!mainEngine->renderState_freeview);
    assert(mainEngine->renderState_live);
    assert(mainEngine->renderState_live->raycastResult->noDims == depth->noDims);

    dump::ReadImageFromFile(expectedRaycastResult, "Tests\\TestFountain\\live.raycastResult.dump");
    dump::ReadImageFromFile(expectedRaycastResult2, "Tests\\TestFountain\\freeview.raycastResult.dump");
    assert(!checkImageSame(expectedRaycastResult, expectedRaycastResult2));

    dump::ReadImageFromFile(expectedRaycastResult, "Tests\\TestFountain\\live.raycastResult.dump");
    assertImageSame(expectedRaycastResult, mainEngine->renderState_live->raycastResult);

    auto imgSize = Vector2i(640, 480);
    ITMPose* pose = new ITMPose();
    auto render = new ITMUChar4Image(imgSize);
    mainEngine->GetImage(render, pose, new ITMIntrinsics(), "renderGrey");

    assertImageSame(render, image("Tests\\TestFountain\\render.png"));
    
    dump::ReadImageFromFile(expectedRaycastResult, "Tests\\TestFountain\\freeview.raycastResult.dump");
    assertImageSame(expectedRaycastResult, mainEngine->renderState_freeview->raycastResult);

    delete rgb;
    delete depth;
    delete mainEngine;
    delete expectedRaycastResult;
}

KERNEL writeImage(Image<char>* image) {
    assert(image->noDims.area() == 1);
    image->GetData()[0] = 42;
}
KERNEL readImage(Image<char>* image,int val) {
    assert(image->GetData()[0] == val);
}
void testImage() {
    auto_ptr<Image<char>> image(new Image<char>());
    const auto* cimage = image.get();
    assert(image->noDims.area() == 1);
    // write gpu
    writeImage << <1, 1 >> >(image.get());
    cudaDeviceSynchronize();
    assert(image->dirtyGPU);

    assert(cimage->GetData()[0] == 42); // must use cimage, otherwise image might be dirty and we forgot update!
    readImage << <1, 1 >> >(image.get(), 42);
    cudaDeviceSynchronize();

    // Write cpu
    image->GetData()[0] = 30;
    image->UpdateDeviceFromHost();// // must manually update device from host because GPU code cannot request this
    readImage << <1, 1 >> >(image.get(), 30);
    cudaDeviceSynchronize();


    image->ChangeDims(Vector2i(10, 10));
    readImage << <1, 1 >> >(image.get(), 0);
    cudaDeviceSynchronize();
}

/// Alternative/external implementation of axis-angle rotation matrix construction
/// axis does not need to be normalized.
Matrix3f createRotation(const Vector3f & _axis, float angle)
{
    Vector3f axis = normalize(_axis);
    float si = sinf(angle);
    float co = cosf(angle);

    Matrix3f ret;
    ret.setIdentity();

    ret *= co;
    for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) ret.at(c, r) += (1.0f - co) * axis[c] * axis[r];

    Matrix3f skewmat;
    skewmat.setZeros();
    skewmat.at(1, 0) = -axis.z;
    skewmat.at(0, 1) = axis.z;
    skewmat.at(2, 0) = axis.y;
    skewmat.at(0, 2) = -axis.y;
    skewmat.at(2, 1) = axis.x; // should be -axis.x;
    skewmat.at(1, 2) = -axis.x;// should be axis.x;
    skewmat *= si;
    ret += skewmat;

    return ret;
}

void testPose() {
    Matrix3f m = createRotation(Vector3f(0, 0, 0), 0);
    Matrix3f id; id.setIdentity();
    approxEqual(m, id);

    {
        Matrix3f rot = createRotation(Vector3f(0, 0, 1), M_PI);
        Matrix3f rot_ = {
            -1, 0, 0,
            0, -1, 0,
            0, 0, 1
        };
        ITMPose pose(0, 0, 0,
            0, 0, M_PI);
        approxEqual(rot, rot_);
        approxEqual(rot, pose.GetR());
    }
    {
#define ran (rand() / (1.f * RAND_MAX))
        Vector3f axis(ran, ran, ran);
        axis = axis.normalised(); // axis must have unit length for itmPose
        float angle = rand() / (1.f * RAND_MAX);

        ITMPose pose(0, 0, 0,
            axis.x*angle, axis.y*angle, axis.z*angle);

        Matrix3f rot = createRotation(axis, angle);
        approxEqual(rot, pose.GetR());
#undef ran
    }
}

void testMatrix() {
    // Various ways of accessing matrix elements 
    Matrix4f m;
    m.setZeros();
    // at(x,y), mxy (not myx!, i.e. both syntaxes give the column first, then the row, different from standard maths)
    m.at(1, 0) = 1;
    m.at(0, 1) = 2;

    Matrix4f n;
    n.setZeros();
    n.m10 = 1;
    n.m01 = 2;
    /* m = n =
    0 1 0 0 
    2 0 0 0 
    0 0 0 0
    0 0 0 0*/

    approxEqual(m, n);

    Vector4f v(1,8,1,2);
    assert(m*v == Vector4f(8,2,0,0));
    assert(n*v == Vector4f(8, 2, 0, 0));
}
// TODO take the tests apart, clean state inbetween
void tests() {
    testMatrix();
    //testPose();
    testImage();
    //testMainEngineProcessFrame();
    testMemblock();
    testImageSource();
    //testTracker();
    //testMainEngine();

    assert(!checkImageSame(image("Tests\\TestRender\\wall.png"), image("Tests\\TestRender\\black.png")));
    assert(!dump::ReadImageFromFile(new ITMUChar4Image(Vector2i(1, 1)), "thisimagedoesnotexist"));
    assert(!png::ReadImageFromFile(new ITMUChar4Image(Vector2i(1, 1)), "thisimagedoesnotexist"));
    assert(!png::SaveImageToFile(new ITMUChar4Image(Vector2i(1, 1)), "C:\\cannotSaveHere.png"));
    testImageSame();
    testDump();
    testForEachPixelNoImage();
    testRenderBlack();
    testRenderWall();
    testScene();
    testCholesky();
    testZ3Hasher();
    testNHasher();
    testZeroHasher();
    //testAllocRequests();
    //testAllocRequests2();

    puts("==== All tests passed ====");
}
